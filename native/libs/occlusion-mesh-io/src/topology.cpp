#include "occlusion/mesh_io/mesh_io.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace occlusion::mesh_io {
namespace {
struct Edge final {
  std::size_t low;
  std::size_t high;
  std::size_t face;
  std::size_t low_corner;
  std::size_t high_corner;
  bool forward;
};

class DisjointSet final {
public:
  explicit DisjointSet(std::size_t count) : parent_(count), rank_(count, 0) {
    std::iota(parent_.begin(), parent_.end(), 0);
  }
  std::size_t find(std::size_t node) {
    if (parent_[node] != node)
      parent_[node] = find(parent_[node]);
    return parent_[node];
  }
  void join(std::size_t a, std::size_t b) {
    a = find(a);
    b = find(b);
    if (a == b)
      return;
    if (rank_[a] < rank_[b])
      std::swap(a, b);
    parent_[b] = a;
    if (rank_[a] == rank_[b])
      ++rank_[a];
  }

private:
  std::vector<std::size_t> parent_;
  std::vector<unsigned char> rank_;
};

MeshIngestionError topology_error(std::string field, std::string message) {
  return {MeshIngestionErrorCode::invalid_topology, 0, std::move(field), std::move(message)};
}
} // namespace

std::optional<MeshIngestionError> validate_closed_oriented_mesh(const TriangleMesh& mesh) {
  if (mesh.vertices_meters.empty() || mesh.triangles.empty())
    return topology_error("mesh", "a closed volume needs vertices and faces");
  if (mesh.vertices_meters.size() > default_maximum_vertices ||
      mesh.triangles.size() > default_maximum_triangles)
    return MeshIngestionError{MeshIngestionErrorCode::resource_limit, 0, "mesh",
                              "exceeds topology limits"};

  std::vector<Edge> edges;
  edges.reserve(mesh.triangles.size() * 3);
  for (std::size_t face = 0; face < mesh.triangles.size(); ++face) {
    const auto& triangle = mesh.triangles[face];
    for (std::size_t corner = 0; corner < 3; ++corner) {
      const auto from = triangle[corner];
      const auto next = (corner + 1) % 3;
      const auto to = triangle[next];
      if (from >= mesh.vertices_meters.size() || to >= mesh.vertices_meters.size() || from == to)
        return topology_error("triangles[" + std::to_string(face) + "]",
                              "face references an invalid vertex or edge");
      const bool forward = from < to;
      edges.push_back({std::min(from, to), std::max(from, to), face,
                       face * 3 + (forward ? corner : next), face * 3 + (forward ? next : corner),
                       forward});
    }
  }

  std::sort(edges.begin(), edges.end(), [](const Edge& a, const Edge& b) {
    return a.low < b.low || (a.low == b.low && a.high < b.high);
  });
  DisjointSet corners(mesh.triangles.size() * 3);
  DisjointSet faces(mesh.triangles.size());
  for (std::size_t start = 0; start < edges.size();) {
    std::size_t end = start + 1;
    while (end < edges.size() && edges[end].low == edges[start].low &&
           edges[end].high == edges[start].high)
      ++end;
    if (end - start != 2)
      return topology_error("edges[" + std::to_string(edges[start].low) + "," +
                                std::to_string(edges[start].high) + "]",
                            "edge must have exactly two incident faces");
    const auto& first = edges[start];
    const auto& second = edges[start + 1];
    if (first.forward == second.forward || first.face == second.face)
      return topology_error("edges[" + std::to_string(first.low) + "," +
                                std::to_string(first.high) + "]",
                            "incident faces must use opposite edge directions");
    corners.join(first.low_corner, second.low_corner);
    corners.join(first.high_corner, second.high_corner);
    faces.join(first.face, second.face);
    start = end;
  }

  const auto unset = std::numeric_limits<std::size_t>::max();
  std::vector<std::size_t> vertex_fan(mesh.vertices_meters.size(), unset);
  for (std::size_t face = 0; face < mesh.triangles.size(); ++face) {
    for (std::size_t corner = 0; corner < 3; ++corner) {
      const auto vertex = mesh.triangles[face][corner];
      const auto root = corners.find(face * 3 + corner);
      if (vertex_fan[vertex] == unset)
        vertex_fan[vertex] = root;
      else if (vertex_fan[vertex] != root)
        return topology_error("vertices[" + std::to_string(vertex) + "]",
                              "vertex has disconnected face fans");
    }
  }
  for (std::size_t vertex = 0; vertex < vertex_fan.size(); ++vertex)
    if (vertex_fan[vertex] == unset)
      return topology_error("vertices[" + std::to_string(vertex) + "]",
                            "vertex is not referenced by a face");

  std::vector<std::size_t> origins(mesh.triangles.size(), unset);
  std::vector<long double> volumes(mesh.triangles.size(), 0);
  for (std::size_t face = 0; face < mesh.triangles.size(); ++face) {
    const auto component = faces.find(face);
    const auto& triangle = mesh.triangles[face];
    if (origins[component] == unset)
      origins[component] = triangle[0];
    const auto& origin = mesh.vertices_meters[origins[component]];
    const auto& av = mesh.vertices_meters[triangle[0]];
    const auto& bv = mesh.vertices_meters[triangle[1]];
    const auto& cv = mesh.vertices_meters[triangle[2]];
    const long double ax = static_cast<long double>(av.x) - origin.x;
    const long double ay = static_cast<long double>(av.y) - origin.y;
    const long double az = static_cast<long double>(av.z) - origin.z;
    const long double bx = static_cast<long double>(bv.x) - origin.x;
    const long double by = static_cast<long double>(bv.y) - origin.y;
    const long double bz = static_cast<long double>(bv.z) - origin.z;
    const long double cx = static_cast<long double>(cv.x) - origin.x;
    const long double cy = static_cast<long double>(cv.y) - origin.y;
    const long double cz = static_cast<long double>(cv.z) - origin.z;
    volumes[component] +=
        (ax * (by * cz - bz * cy) + ay * (bz * cx - bx * cz) + az * (bx * cy - by * cx)) / 6;
  }
  for (std::size_t component = 0; component < volumes.size(); ++component)
    if (origins[component] != unset &&
        (!std::isfinite(volumes[component]) || volumes[component] <= 0))
      return topology_error("mesh", "each shell needs positive outward signed volume");
  return std::nullopt;
}
} // namespace occlusion::mesh_io
