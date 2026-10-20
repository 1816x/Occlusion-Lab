#include "mesh_files.hpp"

#include "occlusion/collision/collision.hpp"
#include "occlusion/mesh_io/mesh_io.hpp"
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <utility>

namespace occlusion::cli {
namespace {
using occlusion::collision::TriangleMesh;
using occlusion::mesh_io::default_maximum_input_bytes;

bool valid_name(std::string_view name) {
  if (name.empty() || name == "." || name == ".." || name.find('/') != name.npos ||
      name.find('\\') != name.npos || name.find(':') != name.npos || name.find('\0') != name.npos)
    return false;
  const std::filesystem::path path{std::string{name}};
  return path.filename() == path && path.extension() == ".obj";
}

std::optional<TriangleMesh> load_mesh(const std::filesystem::path& directory, std::string_view name,
                                      std::string& error) {
  if (!valid_name(name)) {
    error = "mesh names must be direct .obj filenames without path separators";
    return std::nullopt;
  }
  const auto path = directory / std::string{name};
  std::error_code status_error;
  const auto status = std::filesystem::symlink_status(path, status_error);
  if (status_error || !std::filesystem::is_regular_file(status)) {
    error = "mesh must be an existing regular file, not a symlink: " + std::string{name};
    return std::nullopt;
  }
  const auto size = std::filesystem::file_size(path, status_error);
  if (status_error || size > default_maximum_input_bytes) {
    error = "mesh exceeds the 16 MiB input limit or cannot be sized: " + std::string{name};
    return std::nullopt;
  }
  std::ifstream stream(path, std::ios::binary);
  if (!stream) {
    error = "cannot open mesh: " + std::string{name};
    return std::nullopt;
  }
  std::string bytes;
  bytes.resize(default_maximum_input_bytes + 1);
  stream.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  const auto count = stream.gcount();
  if (stream.bad() || (stream.fail() && !stream.eof())) {
    error = "cannot read mesh: " + std::string{name};
    return std::nullopt;
  }
  if (count > static_cast<std::streamsize>(default_maximum_input_bytes)) {
    error = "mesh exceeds the 16 MiB input limit: " + std::string{name};
    return std::nullopt;
  }
  bytes.resize(static_cast<std::size_t>(count));
  const auto parsed = occlusion::mesh_io::parse_obj_mesh(bytes);
  if (!parsed) {
    const auto& problem = parsed.error();
    error = std::string{name} + ": line " + std::to_string(problem.line) + ", " + problem.field +
            ": " + problem.message;
    return std::nullopt;
  }
  TriangleMesh converted;
  converted.vertices_meters = parsed.value().vertices_meters;
  converted.triangles.reserve(parsed.value().triangles.size());
  for (const auto& triangle : parsed.value().triangles) {
    // FCL stores indices as uint32_t; never narrow a parser-owned size_t.
    if (triangle[0] > std::numeric_limits<std::uint32_t>::max() ||
        triangle[1] > std::numeric_limits<std::uint32_t>::max() ||
        triangle[2] > std::numeric_limits<std::uint32_t>::max()) {
      error = std::string{name} + ": triangle index exceeds the collision model range";
      return std::nullopt;
    }
    converted.triangles.push_back({static_cast<std::uint32_t>(triangle[0]),
                                   static_cast<std::uint32_t>(triangle[1]),
                                   static_cast<std::uint32_t>(triangle[2])});
  }
  return converted;
}
} // namespace

std::optional<occlusion::evaluation::PoseEvaluator>
load_mesh_pair(const std::filesystem::path& directory, std::string_view fixed_name,
               std::string_view moving_name, std::string& error) {
  std::error_code status_error;
  if (!std::filesystem::is_directory(directory, status_error) || status_error) {
    error = "mesh directory does not exist or is not a directory";
    return std::nullopt;
  }
  auto fixed = load_mesh(directory, fixed_name, error);
  if (!fixed)
    return std::nullopt;
  auto moving = load_mesh(directory, moving_name, error);
  if (!moving)
    return std::nullopt;
  auto evaluator =
      occlusion::evaluation::PoseEvaluator::create(std::move(*fixed), std::move(*moving));
  if (!evaluator) {
    const auto& problem = evaluator.errors().front();
    error = problem.field + ": " + problem.message;
    return std::nullopt;
  }
  return evaluator.value();
}
} // namespace occlusion::cli
