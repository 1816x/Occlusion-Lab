#include "occlusion/collision/collision.hpp"
#include <cmath>
#include <fstream>
#include <gtest/gtest.h>
#include <limits>
#include <nlohmann/json.hpp>
using namespace occlusion::collision;
using namespace occlusion::core;
namespace {
TriangleMesh box(double horizontal_half = 1.0) {
  return {{{-horizontal_half, -1, -horizontal_half},
           {horizontal_half, -1, -horizontal_half},
           {horizontal_half, 1, -horizontal_half},
           {-horizontal_half, 1, -horizontal_half},
           {-horizontal_half, -1, horizontal_half},
           {horizontal_half, -1, horizontal_half},
           {horizontal_half, 1, horizontal_half},
           {-horizontal_half, 1, horizontal_half}},
          {{0, 2, 1},
           {0, 3, 2},
           {4, 5, 6},
           {4, 6, 7},
           {0, 1, 5},
           {0, 5, 4},
           {3, 7, 6},
           {3, 6, 2},
           {0, 4, 7},
           {0, 7, 3},
           {1, 2, 6},
           {1, 6, 5}}};
}
RigidTransform translated(double y) { return {{0, y, 0}, {1, 0, 0, 0, 1, 0, 0, 0, 1}}; }
CollisionModel compiled(CollisionEngine& engine, double horizontal_half = 1.0) {
  auto result = engine.compile(box(horizontal_half));
  EXPECT_TRUE(result);
  return result.value();
}
} // namespace
TEST(CollisionCompile, AcceptsClosedMesh) {
  CollisionEngine e;
  auto r = e.compile(box());
  ASSERT_TRUE(r);
  EXPECT_EQ(r.value().triangle_count(), 12);
}
TEST(CollisionCompile, RejectsEmptyVerticesAndTriangles) {
  CollisionEngine e;
  auto r = e.compile({});
  ASSERT_FALSE(r);
  EXPECT_EQ(r.errors().size(), 2);
}
TEST(CollisionCompile, RejectsNonFiniteVertex) {
  auto m = box();
  m.vertices_meters[0].x = std::numeric_limits<double>::infinity();
  EXPECT_FALSE(CollisionEngine{}.compile(std::move(m)));
}
TEST(CollisionCompile, RejectsOutOfRangeIndex) {
  auto m = box();
  m.triangles[0].a = 99;
  EXPECT_FALSE(CollisionEngine{}.compile(std::move(m)));
}
TEST(CollisionCompile, RejectsRepeatedIndex) {
  auto m = box();
  m.triangles[0].a = m.triangles[0].b;
  EXPECT_FALSE(CollisionEngine{}.compile(std::move(m)));
}
TEST(CollisionCompile, RejectsDegenerateTriangle) {
  auto m = box();
  m.vertices_meters[m.triangles[0].a] = m.vertices_meters[m.triangles[0].b];
  EXPECT_FALSE(CollisionEngine{}.compile(std::move(m)));
}
TEST(CollisionQuery, RejectsInvalidTransform) {
  CollisionEngine e;
  auto model = compiled(e);
  auto t = translated(3);
  t.rotation[0] = 2;
  EXPECT_FALSE(e.query(model, model, t));
}
TEST(CollisionQuery, ReusesCompiledModelsAcrossTransforms) {
  CollisionEngine e;
  auto fixed = compiled(e), moving = compiled(e, 0.75);
  for (double y : {3., 2., 1.5, 3.})
    EXPECT_TRUE(e.query(fixed, moving, translated(y)));
  EXPECT_EQ(fixed.triangle_count(), 12);
  EXPECT_EQ(moving.triangle_count(), 12);
}
TEST(CollisionQuery, ReportsSeparatedDistance) {
  CollisionEngine e;
  auto fixed = compiled(e), moving = compiled(e, 0.75);
  auto r = e.query(fixed, moving, translated(3));
  ASSERT_TRUE(r);
  EXPECT_EQ(r.value().classification, ContactClassification::separated);
  EXPECT_NEAR(r.value().clearance.value(), 1, 1e-9);
  EXPECT_FALSE(r.value().intersects);
}
TEST(CollisionQuery, ReportsExactTouching) {
  CollisionEngine e;
  auto fixed = compiled(e), moving = compiled(e, 0.75);
  auto r = e.query(fixed, moving, translated(2));
  ASSERT_TRUE(r);
  EXPECT_EQ(r.value().classification, ContactClassification::touching);
  EXPECT_EQ(r.value().penetration_depth.value(), 0);
}
TEST(CollisionQuery, NormalizesTolerance) {
  CollisionEngine e;
  auto fixed = compiled(e), moving = compiled(e, 0.75);
  for (double delta : {5e-7, -5e-7}) {
    auto r = e.query(fixed, moving, translated(2 + delta));
    ASSERT_TRUE(r);
    EXPECT_EQ(r.value().classification, ContactClassification::touching);
  }
}
TEST(CollisionQuery, ReportsPenetration) {
  CollisionEngine e;
  auto fixed = compiled(e), moving = compiled(e, 0.75);
  auto r = e.query(fixed, moving, translated(1.9));
  ASSERT_TRUE(r);
  EXPECT_EQ(r.value().classification, ContactClassification::penetrating);
  EXPECT_GT(r.value().penetration_depth.value(), contact_tolerance.value());
  EXPECT_TRUE(r.value().intersects);
}
TEST(CollisionQuery, UsesMaximumFinitePenetration) {
  CollisionEngine e;
  auto fixed = compiled(e), moving = compiled(e, 0.75);
  auto r = e.query(fixed, moving, translated(1.5));
  ASSERT_TRUE(r);
  EXPECT_NEAR(r.value().penetration_depth.value(), .5, 1e-9);
}
TEST(CollisionQuery, IsDeterministic) {
  CollisionEngine e;
  auto fixed = compiled(e), moving = compiled(e, 0.75);
  auto a = e.query(fixed, moving, translated(1.5));
  auto b = e.query(fixed, moving, translated(1.5));
  ASSERT_TRUE(a && b);
  EXPECT_EQ(a.value().classification, b.value().classification);
  EXPECT_EQ(a.value().penetration_depth.value(), b.value().penetration_depth.value());
}
TEST(CollisionContract, InvariantsHold) {
  CollisionEngine e;
  auto fixed = compiled(e), moving = compiled(e, 0.75);
  for (double y : {3., 2., 1.5}) {
    auto r = e.query(fixed, moving, translated(y));
    ASSERT_TRUE(r);
    const auto& v = r.value();
    if (v.classification == ContactClassification::separated) {
      EXPECT_GT(v.clearance.value(), contact_tolerance.value());
      EXPECT_EQ(v.penetration_depth.value(), 0);
      EXPECT_FALSE(v.intersects);
    } else if (v.classification == ContactClassification::penetrating) {
      EXPECT_EQ(v.clearance.value(), 0);
      EXPECT_GT(v.penetration_depth.value(), contact_tolerance.value());
      EXPECT_TRUE(v.intersects);
    } else {
      EXPECT_EQ(v.clearance.value(), 0);
      EXPECT_EQ(v.penetration_depth.value(), 0);
    }
  }
}
TEST(CollisionFixture, EveryCaseExecutesOnce) {
  std::ifstream stream(OCCLUSION_COLLISION_FIXTURE_PATH);
  nlohmann::json fixture;
  stream >> fixture;
  ASSERT_EQ(fixture["cases"].size(), 7);
  CollisionEngine e;
  auto parse = [](const nlohmann::json& mesh) {
    TriangleMesh value;
    for (const auto& v : mesh["verticesMeters"])
      value.vertices_meters.push_back({v[0], v[1], v[2]});
    for (const auto& t : mesh["triangles"])
      value.triangles.push_back({t[0], t[1], t[2]});
    return value;
  };
  auto fixed = e.compile(parse(fixture["geometry"]["fixed"]));
  auto moving = e.compile(parse(fixture["geometry"]["moving"]));
  ASSERT_TRUE(fixed && moving);
  std::size_t executed = 0;
  for (const auto& c : fixture["cases"]) {
    auto r = e.query(fixed.value(), moving.value(),
                     translated(c["movingTransform"]["translationMeters"]["y"]));
    ASSERT_TRUE(r) << c["id"];
    const std::string expected = c["classification"];
    EXPECT_EQ(static_cast<int>(r.value().classification), expected == "separated"  ? 0
                                                          : expected == "touching" ? 1
                                                                                   : 2)
        << c["id"];
    EXPECT_NEAR(r.value().clearance.value(), c["clearanceMeters"].get<double>(), 1e-9);
    EXPECT_NEAR(r.value().penetration_depth.value(), c["penetrationDepthMeters"].get<double>(),
                1e-9);
    ++executed;
  }
  EXPECT_EQ(executed, fixture["cases"].size());
}
TEST(CollisionFixture, RejectsMalformedFixture) {
  EXPECT_THROW(nlohmann::json::parse("{broken"), nlohmann::json::parse_error);
}
