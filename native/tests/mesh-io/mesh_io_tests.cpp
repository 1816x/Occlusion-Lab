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
}

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
