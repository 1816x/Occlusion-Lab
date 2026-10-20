#include "occlusion/mesh_io/mesh_io.hpp"

#include <gtest/gtest.h>
#include <limits>
#include <string>

using namespace occlusion::mesh_io;

namespace {
constexpr std::string_view triangle = R"obj(# coordinates are meters
v 0 0 0
v 1 0 0
v 0 1 0
f 1 2 3
)obj";

TriangleMesh closed_box() {
  TriangleMesh mesh;
  mesh.vertices_meters = {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
                          {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}};
  mesh.triangles = {{0, 2, 1}, {0, 3, 2}, {4, 5, 6}, {4, 6, 7}, {0, 1, 5}, {0, 5, 4},
                    {3, 7, 6}, {3, 6, 2}, {0, 4, 7}, {0, 7, 3}, {1, 2, 6}, {1, 6, 5}};
  return mesh;
}
} // namespace

TEST(ObjIngestion, ParsesStrictTriangleSubsetAndConvertsIndices) {
  const auto result = parse_obj_mesh(triangle);
  ASSERT_TRUE(result);
  ASSERT_EQ(result.value().vertices_meters.size(), 3);
  ASSERT_EQ(result.value().triangles.size(), 1);
  EXPECT_DOUBLE_EQ(result.value().vertices_meters[1].x, 1);
  EXPECT_EQ(result.value().triangles[0], (std::array<std::size_t, 3>{0, 1, 2}));
}

TEST(ObjIngestion, AcceptsCrLfWhitespaceAndInlineComments) {
  const auto result =
      parse_obj_mesh("  v 0 0 0\r\n\tv 1 0 0 # x\r\nv 0 1 0\r\nf 1 2 3 # triangle\r\n");
  ASSERT_TRUE(result);
  EXPECT_EQ(result.value().vertices_meters.size(), 3);
}

TEST(ObjIngestion, RejectsEmptyAndIncompleteMeshes) {
  auto empty = parse_obj_mesh("");
  ASSERT_FALSE(empty);
  EXPECT_EQ(empty.error().code, MeshIngestionErrorCode::empty_input);
  auto vertices_only = parse_obj_mesh("v 0 0 0\n");
  ASSERT_FALSE(vertices_only);
  EXPECT_EQ(vertices_only.error().code, MeshIngestionErrorCode::empty_mesh);
}

TEST(ObjIngestion, RejectsUnsupportedStatementsAndObjIndexVariants) {
  auto normal = parse_obj_mesh("vn 0 1 0\n");
  ASSERT_FALSE(normal);
  EXPECT_EQ(normal.error().code, MeshIngestionErrorCode::unsupported_statement);
  auto slash = parse_obj_mesh("v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1/1 2/2 3/3\n");
  ASSERT_FALSE(slash);
  EXPECT_EQ(slash.error().code, MeshIngestionErrorCode::invalid_face);
  auto polygon = parse_obj_mesh("v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nf 1 2 3 4\n");
  ASSERT_FALSE(polygon);
  EXPECT_EQ(polygon.error().code, MeshIngestionErrorCode::invalid_face);
}

TEST(ObjIngestion, RejectsMalformedAndNonFiniteCoordinates) {
  auto malformed = parse_obj_mesh("v one 0 0\n");
  ASSERT_FALSE(malformed);
  EXPECT_EQ(malformed.error().code, MeshIngestionErrorCode::invalid_number);
  auto non_finite = parse_obj_mesh("v nan 0 0\n");
  ASSERT_FALSE(non_finite);
  EXPECT_EQ(non_finite.error().code, MeshIngestionErrorCode::non_finite_coordinate);
}

TEST(ObjIngestion, RejectsInvalidIndicesBeforePublishingMesh) {
  auto forward = parse_obj_mesh("v 0 0 0\nf 1 2 3\nv 1 0 0\nv 0 1 0\n");
  ASSERT_FALSE(forward);
  EXPECT_EQ(forward.error().code, MeshIngestionErrorCode::index_out_of_range);
  EXPECT_EQ(forward.error().line, 2);
  auto repeated = parse_obj_mesh("v 0 0 0\nv 1 0 0\nf 1 2 2\n");
  ASSERT_FALSE(repeated);
  EXPECT_EQ(repeated.error().code, MeshIngestionErrorCode::invalid_face);
}

TEST(ObjIngestion, EnforcesEveryResourceLimit) {
  auto bytes = parse_obj_mesh(triangle, {.maximum_input_bytes = 4});
  ASSERT_FALSE(bytes);
  EXPECT_EQ(bytes.error().code, MeshIngestionErrorCode::resource_limit);
  auto vertices = parse_obj_mesh(triangle, {.maximum_vertices = 2});
  ASSERT_FALSE(vertices);
  EXPECT_EQ(vertices.error().field, "vertices");
  auto faces = parse_obj_mesh(triangle, {.maximum_triangles = 0});
  ASSERT_FALSE(faces);
  EXPECT_EQ(faces.error().field, "triangles");
}

TEST(ObjTopology, AcceptsClosedOutwardBox) {
  EXPECT_FALSE(validate_closed_oriented_mesh(closed_box()));
}

TEST(ObjTopology, RejectsOpenAndNonManifoldEdges) {
  auto open = closed_box();
  open.triangles.pop_back();
  const auto boundary = validate_closed_oriented_mesh(open);
  ASSERT_TRUE(boundary);
  EXPECT_EQ(boundary->code, MeshIngestionErrorCode::invalid_topology);
  EXPECT_EQ(boundary->line, 0);
  EXPECT_EQ(boundary->field.substr(0, 6), "edges[");

  auto non_manifold = closed_box();
  non_manifold.triangles.push_back(non_manifold.triangles.front());
  const auto shared_by_three = validate_closed_oriented_mesh(non_manifold);
  ASSERT_TRUE(shared_by_three);
  EXPECT_EQ(shared_by_three->code, MeshIngestionErrorCode::invalid_topology);
  EXPECT_EQ(shared_by_three->field.substr(0, 6), "edges[");
}

TEST(ObjTopology, RejectsInconsistentAndInwardWinding) {
  auto mixed = closed_box();
  std::swap(mixed.triangles.front()[1], mixed.triangles.front()[2]);
  const auto inconsistent = validate_closed_oriented_mesh(mixed);
  ASSERT_TRUE(inconsistent);
  EXPECT_EQ(inconsistent->field.substr(0, 6), "edges[");

  auto inward = closed_box();
  for (auto& face : inward.triangles)
    std::swap(face[1], face[2]);
  const auto negative_volume = validate_closed_oriented_mesh(inward);
  ASSERT_TRUE(negative_volume);
  EXPECT_EQ(negative_volume->field, "mesh");
  EXPECT_NE(negative_volume->message.find("outward"), std::string::npos);
}

TEST(ObjTopology, RejectsDisconnectedVertexFans) {
  auto mesh = closed_box();
  auto second = closed_box();
  // The second box touches the first at exactly one shared vertex.
  std::array<std::size_t, 8> mapping{};
  for (std::size_t i = 0; i < second.vertices_meters.size(); ++i) {
    if (i == 6) {
      mapping[i] = 0;
    } else {
      mapping[i] = mesh.vertices_meters.size();
      const auto& vertex = second.vertices_meters[i];
      mesh.vertices_meters.push_back({vertex.x - 2, vertex.y - 2, vertex.z - 2});
    }
  }
  for (const auto& face : second.triangles)
    mesh.triangles.push_back({mapping[face[0]], mapping[face[1]], mapping[face[2]]});
  const auto problem = validate_closed_oriented_mesh(mesh);
  ASSERT_TRUE(problem);
  EXPECT_EQ(problem->field, "vertices[0]");
}

TEST(ObjTopology, RejectsUnreferencedVertices) {
  auto mesh = closed_box();
  mesh.vertices_meters.push_back({2, 2, 2});
  const auto problem = validate_closed_oriented_mesh(mesh);
  ASSERT_TRUE(problem);
  EXPECT_EQ(problem->field, "vertices[8]");
}
