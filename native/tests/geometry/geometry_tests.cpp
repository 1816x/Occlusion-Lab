#include "occlusion/geometry/geometry.hpp"

#include <gtest/gtest.h>
#include <limits>

namespace {
occlusion::mesh_io::TriangleMesh box() {
  occlusion::mesh_io::TriangleMesh mesh;
  mesh.vertices_meters = {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
                          {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}};
  mesh.triangles = {{0, 2, 1}, {0, 3, 2}, {4, 5, 6}, {4, 6, 7}, {0, 1, 5}, {0, 5, 4},
                    {3, 7, 6}, {3, 6, 2}, {0, 4, 7}, {0, 7, 3}, {1, 2, 6}, {1, 6, 5}};
  return mesh;
}
} // namespace

TEST(Geometry, AcceptsClosedNonIntersectingBox) {
  EXPECT_FALSE(occlusion::geometry::inspect_mesh(box()));
}

TEST(Geometry, RejectsOpenMeshBeforeConversion) {
  auto mesh = box();
  mesh.triangles.pop_back();
  const auto problem = occlusion::geometry::inspect_mesh(mesh);
  ASSERT_TRUE(problem);
  EXPECT_EQ(problem->code, occlusion::geometry::GeometryErrorCode::invalid_mesh);
  EXPECT_EQ(problem->field.substr(0, 6), "edges[");
}

TEST(Geometry, RejectsDegenerateFace) {
  auto mesh = box();
  mesh.vertices_meters[1] = mesh.vertices_meters[0];
  const auto problem = occlusion::geometry::inspect_mesh(mesh);
  ASSERT_TRUE(problem);
  EXPECT_EQ(problem->code, occlusion::geometry::GeometryErrorCode::degenerate_face);
  EXPECT_EQ(problem->field, "triangles[0]");
}

TEST(Geometry, RejectsIntersectingClosedShells) {
  auto mesh = box();
  const auto other = box();
  const auto offset = mesh.vertices_meters.size();
  for (auto point : other.vertices_meters)
    mesh.vertices_meters.push_back({point.x + 0.5, point.y + 0.5, point.z + 0.5});
  for (const auto& face : other.triangles)
    mesh.triangles.push_back({face[0] + offset, face[1] + offset, face[2] + offset});
  const auto problem = occlusion::geometry::inspect_mesh(mesh);
  ASSERT_TRUE(problem);
  EXPECT_EQ(problem->code, occlusion::geometry::GeometryErrorCode::self_intersection);
}

TEST(Geometry, RejectsNonFiniteCoordinatesBeforeCGALConversion) {
  auto mesh = box();
  mesh.vertices_meters[0].x = std::numeric_limits<double>::infinity();
  const auto problem = occlusion::geometry::inspect_mesh(mesh);
  ASSERT_TRUE(problem);
  EXPECT_EQ(problem->code, occlusion::geometry::GeometryErrorCode::invalid_mesh);
  EXPECT_EQ(problem->field, "vertices[0]");
}
