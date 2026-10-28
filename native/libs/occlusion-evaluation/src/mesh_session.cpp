#include "occlusion/evaluation/mesh_session.hpp"

#include <cstdint>
#include <limits>
#include <utility>

namespace occlusion::evaluation {
namespace {
std::optional<collision::TriangleMesh> collision_mesh(const mesh_io::TriangleMesh& source,
                                                      std::string_view filename,
                                                      mesh_io::ObjFileError& error) {
  collision::TriangleMesh converted;
  converted.vertices_meters = source.vertices_meters;
  converted.triangles.reserve(source.triangles.size());
  for (const auto& triangle : source.triangles) {
    if (triangle[0] > std::numeric_limits<std::uint32_t>::max() ||
        triangle[1] > std::numeric_limits<std::uint32_t>::max() ||
        triangle[2] > std::numeric_limits<std::uint32_t>::max()) {
      error = {"index_out_of_range", "triangles",
               "triangle index exceeds the collision model range", 0, std::string{filename}};
      return std::nullopt;
    }
    converted.triangles.push_back({static_cast<std::uint32_t>(triangle[0]),
                                   static_cast<std::uint32_t>(triangle[1]),
                                   static_cast<std::uint32_t>(triangle[2])});
  }
  return converted;
}
} // namespace

std::optional<MeshEvaluationSession>
load_mesh_evaluation_session(const std::filesystem::path& directory, std::string_view fixed_name,
                             std::string_view moving_name, mesh_io::ObjFileError& error) {
  error = {};
  auto fixed = mesh_io::load_synthetic_obj(directory, fixed_name, error);
  if (!fixed)
    return std::nullopt;
  auto moving = mesh_io::load_synthetic_obj(directory, moving_name, error);
  if (!moving)
    return std::nullopt;
  auto fixed_collision = collision_mesh(*fixed, fixed_name, error);
  if (!fixed_collision)
    return std::nullopt;
  auto moving_collision = collision_mesh(*moving, moving_name, error);
  if (!moving_collision)
    return std::nullopt;
  auto evaluator = PoseEvaluator::create(std::move(*fixed_collision), std::move(*moving_collision));
  if (!evaluator) {
    const auto& problem = evaluator.errors().front();
    error = {"invalid_collision_mesh", problem.field, problem.message, 0, {}};
    return std::nullopt;
  }
  return MeshEvaluationSession{std::move(*fixed), std::move(*moving), evaluator.value()};
}
} // namespace occlusion::evaluation
