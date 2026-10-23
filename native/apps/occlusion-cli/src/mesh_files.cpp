#include "mesh_files.hpp"

#include "occlusion/collision/collision.hpp"
#include "occlusion/mesh_io/obj_files.hpp"

#include <cstdint>
#include <limits>
#include <utility>

namespace occlusion::cli {
namespace {
using occlusion::collision::TriangleMesh;

std::optional<TriangleMesh> load_mesh(const std::filesystem::path& directory, std::string_view name,
                                      std::string& error) {
  mesh_io::ObjFileError problem;
  const auto parsed = mesh_io::load_synthetic_obj(directory, name, problem);
  if (!parsed) {
    error = mesh_io::describe_obj_file_error(problem);
    return std::nullopt;
  }
  TriangleMesh converted;
  converted.vertices_meters = parsed->vertices_meters;
  converted.triangles.reserve(parsed->triangles.size());
  for (const auto& triangle : parsed->triangles) {
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
