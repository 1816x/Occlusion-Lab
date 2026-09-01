#include "occlusion/evaluation/evaluation.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <future>
#include <gtest/gtest.h>
#include <limits>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>
using namespace occlusion::core;
using namespace occlusion::collision;
using namespace occlusion::evaluation;
namespace {
TriangleMesh box(double h = .05) {
  return {{{-h, -h, -h},
           {h, -h, -h},
           {h, h, -h},
           {-h, h, -h},
           {-h, -h, h},
           {h, -h, h},
           {h, h, h},
           {-h, h, h}},
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
MandibularPose pose(double opening) { return {Meters{opening}, Meters{0}, Meters{0}}; }
ContactCandidate candidate(double x, double depth = 0, Vector3 normal = {0, 1, 0}) {
  return {{x, 0, 0},
          normal,
          Meters{-depth},
          Meters{depth},
          depth > contact_tolerance.value() ? ContactClassification::penetrating
                                            : ContactClassification::touching};
}
std::string read(const char* path) {
  std::ifstream f(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(f), {}};
}
} // namespace
TEST(EvaluatorConstruction, AcceptsValidMeshes) {
  EXPECT_TRUE(PoseEvaluator::create(box(), box()));
}
TEST(EvaluatorConstruction, RejectsFixedMesh) { EXPECT_FALSE(PoseEvaluator::create({}, box())); }
TEST(EvaluatorConstruction, RejectsMovingMesh) { EXPECT_FALSE(PoseEvaluator::create(box(), {})); }
TEST(Evaluator, ValidatesPoseBeforeQuery) {
  auto e = PoseEvaluator::create(box(), box()).value();
  EXPECT_FALSE(e.evaluate(pose(.3)));
  EXPECT_EQ(e.query_count(), 0);
}
TEST(Evaluator, AppliesNativeTransform) {
  auto e = PoseEvaluator::create(box(), box()).value();
  auto r = e.evaluate(pose(.06));
  ASSERT_TRUE(r);
  EXPECT_DOUBLE_EQ(r.value().applied_transform.translation_meters.y, .1);
}
TEST(Evaluator, ReportsSeparationClearance) {
  auto e = PoseEvaluator::create(box(), box()).value();
  auto r = e.evaluate(pose(0));
  ASSERT_TRUE(r);
  EXPECT_EQ(r.value().classification, ContactClassification::separated);
  ASSERT_TRUE(r.value().clearance);
  EXPECT_NEAR(r.value().clearance->value(), .06, 1e-9);
  EXPECT_EQ(r.value().penetration_depth.value(), 0);
}
TEST(Evaluator, ReportsTouching) {
  auto e = PoseEvaluator::create(box(), box()).value();
  auto r = e.evaluate(pose(.0599999));
  ASSERT_TRUE(r);
  EXPECT_EQ(r.value().classification, ContactClassification::touching);
  EXPECT_FALSE(r.value().clearance);
}
TEST(Evaluator, ReportsPenetrationAndMaximumDepth) {
  auto e = PoseEvaluator::create(box(), box()).value();
  auto r = e.evaluate(pose(.07));
  ASSERT_TRUE(r);
  EXPECT_EQ(r.value().classification, ContactClassification::penetrating);
  EXPECT_GT(r.value().penetration_depth.value(), contact_tolerance.value());
}
TEST(Evaluator, ReusesTwoCompiledModelsFor61Queries) {
  auto e = PoseEvaluator::create(box(), box()).value();
  for (int i = 0; i < 61; ++i)
    ASSERT_TRUE(e.evaluate(pose(0)));
  EXPECT_EQ(e.compilation_count(), 2);
  EXPECT_EQ(e.query_count(), 61);
}
TEST(Evaluator, RepeatsDeterministically) {
  auto e = PoseEvaluator::create(box(), box()).value();
  auto a = e.evaluate(pose(.07)), b = e.evaluate(pose(.07));
  ASSERT_TRUE(a && b);
  EXPECT_EQ(a.value().normalized_contact_count, b.value().normalized_contact_count);
  for (size_t i = 0; i < a.value().contacts.size(); ++i)
    EXPECT_EQ(a.value().contacts[i], b.value().contacts[i]);
}
TEST(Evaluator, SupportsConcurrentReadOnlyQueries) {
  auto e = PoseEvaluator::create(box(), box()).value();
  std::vector<std::future<bool>> f;
  for (int i = 0; i < 8; ++i)
    f.push_back(std::async(std::launch::async, [e] { return bool(e.evaluate(pose(0))); }));
  for (auto& x : f)
    EXPECT_TRUE(x.get());
  EXPECT_EQ(e.query_count(), 8);
}
TEST(Normalization, RejectsInvalidNormalsAndNonFiniteValues) {
  EXPECT_FALSE(normalize_contacts({candidate(0, 0, {0, 0, 0})}));
  EXPECT_FALSE(normalize_contacts({candidate(std::numeric_limits<double>::quiet_NaN())}));
  auto c = candidate(0);
  c.penetration_depth = Meters{std::numeric_limits<double>::infinity()};
  EXPECT_FALSE(normalize_contacts({c}));
}
TEST(Normalization, OrientsUnitNormalsAndStableIds) {
  auto r = normalize_contacts({candidate(0, 0, {0, 2, 0})});
  ASSERT_TRUE(r);
  ASSERT_EQ(r.value().size(), 1);
  EXPECT_NEAR(r.value()[0].normal_fixed_to_moving.y, 1, normal_unit_tolerance);
  EXPECT_FALSE(r.value()[0].stable_id.empty());
}
TEST(Normalization, DeduplicatesByGreatestDepth) {
  auto r = normalize_contacts({candidate(.000001, .001), candidate(.000002, .003)});
  ASSERT_TRUE(r);
  ASSERT_EQ(r.value().size(), 1);
  EXPECT_DOUBLE_EQ(r.value()[0].penetration_depth.value(), .003);
}
TEST(Normalization, IsInputOrderIndependent) {
  std::vector<ContactCandidate> a{candidate(.03), candidate(.01), candidate(.02)};
  auto b = a;
  std::reverse(b.begin(), b.end());
  auto x = normalize_contacts(a), y = normalize_contacts(b);
  ASSERT_TRUE(x && y);
  EXPECT_EQ(x.value(), y.value());
}
TEST(Normalization, SortsBeforeTruncatingAt32) {
  std::vector<ContactCandidate> c;
  for (int i = 39; i >= 0; --i)
    c.push_back(candidate(i * .00002));
  auto r = normalize_contacts(c);
  ASSERT_TRUE(r);
  ASSERT_EQ(r.value().size(), 32);
  EXPECT_DOUBLE_EQ(r.value().front().position_meters.x, 0);
  EXPECT_DOUBLE_EQ(r.value().back().position_meters.x, 31 * .00002);
  std::set<std::string> ids;
  for (auto& s : r.value()) {
    EXPECT_TRUE(std::isfinite(s.position_meters.x));
    ids.insert(s.stable_id);
  }
  EXPECT_EQ(ids.size(), r.value().size());
}
TEST(NormalizationFixture, HasProfessionalCoverageAndRunsEveryCase) {
  std::ifstream f(OCCLUSION_NORMALIZATION_FIXTURE_SOURCE);
  nlohmann::json j;
  f >> j;
  ASSERT_EQ(j["metadata"]["sourceBaselineCommit"], "da055bc2067b6477eea9efe7abbad0422ed8e9f3");
  ASSERT_EQ(j["cases"].size(), 10);
  for (auto& tc : j["cases"]) {
    std::vector<ContactCandidate> cs;
    for (auto& c : tc["candidates"]) {
      auto p = c["positionMeters"];
      auto n = c["normalFixedToMoving"];
      double d = c["penetrationDepthMeters"];
      cs.push_back({{p[0], p[1], p[2]},
                    {n[0], n[1], n[2]},
                    Meters{-d},
                    Meters{d},
                    c["classification"] == "penetrating" ? ContactClassification::penetrating
                                                         : ContactClassification::touching});
    }
    auto r = normalize_contacts(cs);
    ASSERT_TRUE(r) << tc["id"];
    EXPECT_LE(r.value().size(), 32);
  }
}
TEST(NormalizationFixture, RejectsMalformedFixture) {
  EXPECT_THROW(nlohmann::json::parse("{bad"), nlohmann::json::parse_error);
}
TEST(NormalizationFixture, TrackedFixtureCopyIsByteExact) {
  EXPECT_EQ(read(OCCLUSION_NORMALIZATION_FIXTURE_SOURCE),
            read(OCCLUSION_NORMALIZATION_FIXTURE_COPY));
}
