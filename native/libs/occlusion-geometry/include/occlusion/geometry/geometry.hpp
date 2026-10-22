#pragma once

#include "occlusion/mesh_io/mesh_io.hpp"

#include <optional>
#include <string>

namespace occlusion::geometry {
enum class GeometryErrorCode {
  invalid_mesh,
  conversion_failed,
  degenerate_face,
  self_intersection
};

struct GeometryError final {
  GeometryErrorCode code;
  std::string field;
  std::string message;
  [[nodiscard]] bool operator==(const GeometryError&) const = default;
};

// Read-only checks for synthetic, meter-valued meshes. Validates closed,
// outward-oriented topology, converts to CGAL privately, then checks for
// degenerate faces and self-intersections. No mesh is repaired or returned.
[[nodiscard]] std::optional<GeometryError>
inspect_mesh(const occlusion::mesh_io::TriangleMesh& mesh);
} // namespace occlusion::geometry
