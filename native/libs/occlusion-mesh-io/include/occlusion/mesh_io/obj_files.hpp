#pragma once

#include "occlusion/mesh_io/mesh_io.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace occlusion::mesh_io {
struct ObjFileError final {
  std::string code;
  std::string field;
  std::string message;
  std::size_t line = 0;
  std::string filename;
};

// Reads a direct synthetic .obj child of a trusted directory with the same
// path, size, parser, and topology checks for evaluation and inspection.
[[nodiscard]] std::optional<TriangleMesh> load_synthetic_obj(const std::filesystem::path& directory,
                                                             std::string_view name,
                                                             ObjFileError& error);
[[nodiscard]] std::string describe_obj_file_error(const ObjFileError& error);
} // namespace occlusion::mesh_io
