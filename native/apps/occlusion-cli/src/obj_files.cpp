#include "obj_files.hpp"

#include <fstream>
#include <utility>

namespace occlusion::cli {
namespace {
bool valid_name(std::string_view name) {
  if (name.empty() || name == "." || name == ".." || name.find('/') != name.npos ||
      name.find('\\') != name.npos || name.find(':') != name.npos || name.find('\0') != name.npos)
    return false;
  const std::filesystem::path path{std::string{name}};
  return path.filename() == path && path.extension() == ".obj";
}

std::string_view ingestion_code(mesh_io::MeshIngestionErrorCode code) {
  using mesh_io::MeshIngestionErrorCode;
  switch (code) {
  case MeshIngestionErrorCode::empty_input:
    return "empty_input";
  case MeshIngestionErrorCode::resource_limit:
    return "resource_limit";
  case MeshIngestionErrorCode::unsupported_statement:
    return "unsupported_statement";
  case MeshIngestionErrorCode::invalid_number:
    return "invalid_number";
  case MeshIngestionErrorCode::non_finite_coordinate:
    return "non_finite_coordinate";
  case MeshIngestionErrorCode::invalid_face:
    return "invalid_face";
  case MeshIngestionErrorCode::index_out_of_range:
    return "index_out_of_range";
  case MeshIngestionErrorCode::empty_mesh:
    return "empty_mesh";
  case MeshIngestionErrorCode::invalid_topology:
    return "invalid_topology";
  }
  return "unknown_error";
}
} // namespace

std::optional<mesh_io::TriangleMesh> load_synthetic_obj(const std::filesystem::path& directory,
                                                        std::string_view name,
                                                        ObjFileError& error) {
  std::error_code status_error;
  if (!std::filesystem::is_directory(directory, status_error) || status_error) {
    error = {"invalid_directory",
             "directory",
             "mesh directory does not exist or is not a directory",
             0,
             {}};
    return std::nullopt;
  }
  if (!valid_name(name)) {
    error = {"invalid_name",
             "filename",
             "mesh names must be direct .obj filenames without path separators",
             0,
             {}};
    return std::nullopt;
  }
  const auto path = directory / std::string{name};
  const auto status = std::filesystem::symlink_status(path, status_error);
  if (status_error || !std::filesystem::is_regular_file(status)) {
    error = {"invalid_file", "filename", "mesh must be an existing regular file, not a symlink", 0,
             std::string{name}};
    return std::nullopt;
  }
  const auto size = std::filesystem::file_size(path, status_error);
  if (status_error || size > mesh_io::default_maximum_input_bytes) {
    error = {"resource_limit", "file", "mesh exceeds the 16 MiB input limit or cannot be sized", 0,
             std::string{name}};
    return std::nullopt;
  }
  std::ifstream stream(path, std::ios::binary);
  if (!stream) {
    error = {"read_error", "file", "cannot open mesh", 0, std::string{name}};
    return std::nullopt;
  }
  std::string bytes;
  bytes.resize(mesh_io::default_maximum_input_bytes + 1);
  stream.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  const auto count = stream.gcount();
  if (stream.bad() || (stream.fail() && !stream.eof())) {
    error = {"read_error", "file", "cannot read mesh", 0, std::string{name}};
    return std::nullopt;
  }
  if (count > static_cast<std::streamsize>(mesh_io::default_maximum_input_bytes)) {
    error = {"resource_limit", "file", "mesh exceeds the 16 MiB input limit", 0, std::string{name}};
    return std::nullopt;
  }
  bytes.resize(static_cast<std::size_t>(count));
  const auto parsed = mesh_io::parse_obj_mesh(bytes);
  if (!parsed) {
    const auto& problem = parsed.error();
    error = {std::string{ingestion_code(problem.code)}, problem.field, problem.message,
             problem.line, std::string{name}};
    return std::nullopt;
  }
  if (const auto problem = mesh_io::validate_closed_oriented_mesh(parsed.value())) {
    error = {std::string{ingestion_code(problem->code)}, problem->field, problem->message,
             problem->line, std::string{name}};
    return std::nullopt;
  }
  return parsed.value();
}

std::string describe_obj_file_error(const ObjFileError& error) {
  if (error.filename.empty())
    return error.message;
  std::string result = error.filename + ": ";
  if (error.line != 0)
    result += "line " + std::to_string(error.line) + ", ";
  return result + error.field + ": " + error.message;
}
} // namespace occlusion::cli
