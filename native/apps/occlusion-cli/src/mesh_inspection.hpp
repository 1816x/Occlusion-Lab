#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace occlusion::cli {
struct InspectionResponse final {
  bool valid;
  std::string json;
};

// Available only in builds with OCCLUSION_ENABLE_GEOMETRY. The v1 JSON
// document is returned for both valid and rejected synthetic OBJ files.
[[nodiscard]] InspectionResponse inspect_synthetic_obj(const std::filesystem::path& directory,
                                                       std::string_view name);
} // namespace occlusion::cli
