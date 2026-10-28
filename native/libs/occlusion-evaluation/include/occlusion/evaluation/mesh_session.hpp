#pragma once

#include "occlusion/evaluation/evaluation.hpp"
#include "occlusion/mesh_io/obj_files.hpp"

#include <filesystem>
#include <optional>
#include <string_view>

namespace occlusion::evaluation {
// Owns the exact validated meshes used to compile the evaluator. Desktop
// presentation can copy these arrays without reopening files or invoking FCL.
struct MeshEvaluationSession final {
  mesh_io::TriangleMesh fixed_mesh;
  mesh_io::TriangleMesh moving_mesh;
  PoseEvaluator evaluator;
};

[[nodiscard]] std::optional<MeshEvaluationSession>
load_mesh_evaluation_session(const std::filesystem::path& directory, std::string_view fixed_name,
                             std::string_view moving_name, mesh_io::ObjFileError& error);
} // namespace occlusion::evaluation
