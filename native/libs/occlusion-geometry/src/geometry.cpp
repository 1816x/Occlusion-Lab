#include "occlusion/geometry/geometry.hpp"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_mesh_processing/self_intersections.h>
#include <CGAL/Polygon_mesh_processing/shape_predicates.h>
#include <CGAL/Surface_mesh.h>

#include <cmath>
#include <vector>

namespace occlusion::geometry {
namespace {
using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
using SurfaceMesh = CGAL::Surface_mesh<Kernel::Point_3>;
namespace PMP = CGAL::Polygon_mesh_processing;
} // namespace

std::optional<GeometryError> inspect_mesh(const occlusion::mesh_io::TriangleMesh& mesh) {
  for (std::size_t index = 0; index < mesh.vertices_meters.size(); ++index) {
    const auto& point = mesh.vertices_meters[index];
    if (!std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z))
      return GeometryError{GeometryErrorCode::invalid_mesh,
                           "vertices[" + std::to_string(index) + "]", "coordinates must be finite"};
  }
  if (const auto problem = occlusion::mesh_io::validate_closed_oriented_mesh(mesh))
    return GeometryError{GeometryErrorCode::invalid_mesh, problem->field, problem->message};

  SurfaceMesh surface;
  std::vector<SurfaceMesh::Vertex_index> vertices;
  vertices.reserve(mesh.vertices_meters.size());
  for (std::size_t index = 0; index < mesh.vertices_meters.size(); ++index) {
    const auto& point = mesh.vertices_meters[index];
    vertices.push_back(surface.add_vertex(Kernel::Point_3{point.x, point.y, point.z}));
  }

  for (std::size_t index = 0; index < mesh.triangles.size(); ++index) {
    const auto& face = mesh.triangles[index];
    const auto added = surface.add_face(vertices[face[0]], vertices[face[1]], vertices[face[2]]);
    if (added == SurfaceMesh::null_face())
      return GeometryError{GeometryErrorCode::conversion_failed,
                           "triangles[" + std::to_string(index) + "]",
                           "could not insert triangle into CGAL surface mesh"};
    if (PMP::is_degenerate_triangle_face(added, surface))
      return GeometryError{GeometryErrorCode::degenerate_face,
                           "triangles[" + std::to_string(index) + "]", "triangle has zero area"};
  }
  if (PMP::does_self_intersect(surface))
    return GeometryError{GeometryErrorCode::self_intersection, "mesh",
                         "surface has intersecting triangles"};
  return std::nullopt;
}
} // namespace occlusion::geometry
