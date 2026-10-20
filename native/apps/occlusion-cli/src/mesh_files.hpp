#pragma once

#include "occlusion/evaluation/evaluation.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace occlusion::cli {
// Only direct .obj children of the supplied directory are accepted. The
// directory must be trusted against concurrent modification by other users.
[[nodiscard]] std::optional<occlusion::evaluation::PoseEvaluator>
load_mesh_pair(const std::filesystem::path& directory, std::string_view fixed_name,
               std::string_view moving_name, std::string& error);
} // namespace occlusion::cli
