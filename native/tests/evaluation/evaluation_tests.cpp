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
bool same_pose(const MandibularPose& a, const MandibularPose& b) {
  return a.opening.value() == b.opening.value() && a.protrusion.value() == b.protrusion.value() &&
         a.lateral_displacement.value() == b.lateral_displacement.value();
}
EvaluatedSweepFrame summary_frame(std::size_t index, std::size_t count,
                                  ContactClassification classification, double penetration) {
  const auto p = pose(0);
  std::vector<NormalizedContactSample> contacts;
  if (classification != ContactClassification::separated)
    contacts.push_back({"fixture", {0, 0, 0}, {0, 1, 0}, Meters{penetration}, classification});
  PoseEvaluationResult evaluation{p,
                                  {{0, .16, 0}, {1, 0, 0, 0, 1, 0, 0, 0, 1}},
                                  classification,
                                  MeasurementStatus::available,
                                  classification == ContactClassification::separated
                                      ? std::optional<Meters>{Meters{.01}}
                                      : std::nullopt,
                                  Meters{penetration},
                                  classification != ContactClassification::separated,
                                  contacts.size(),
                                  std::move(contacts)};
  return {index, static_cast<double>(index) / static_cast<double>(count - 1), p,
          std::move(evaluation)};
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
TEST(EvaluatedSweep, RejectsLowAndHighCountsWithoutQueries) {
  auto e = PoseEvaluator::create(box(), box()).value();
  EXPECT_FALSE(e.evaluate_sweep(SweepPreset::closing, 1));
  EXPECT_FALSE(e.evaluate_sweep(SweepPreset::closing, 62));
  EXPECT_EQ(e.query_count(), 0);
}
TEST(EvaluatedSweep, RejectsInvalidPresetWithoutQueries) {
  auto e = PoseEvaluator::create(box(), box()).value();
  EXPECT_FALSE(e.evaluate_sweep(static_cast<SweepPreset>(99), 2));
  EXPECT_EQ(e.query_count(), 0);
}
TEST(EvaluatedSweep, EvaluatesEveryPresetAndRequiredFrameCount) {
  for (auto preset : {SweepPreset::closing, SweepPreset::protrusive, SweepPreset::left_lateral,
                      SweepPreset::right_lateral}) {
    for (std::size_t count : {std::size_t{2}, std::size_t{31}, std::size_t{61}}) {
      auto e = PoseEvaluator::create(box(), box()).value();
      auto result = e.evaluate_sweep(preset, count);
      ASSERT_TRUE(result);
      EXPECT_EQ(result.value().frames.size(), count);
      EXPECT_EQ(e.compilation_count(), 2);
      EXPECT_EQ(e.query_count(), count);
      EXPECT_TRUE(
          same_pose(result.value().final_pose, result.value().frames.back().requested_pose));
    }
  }
}
TEST(EvaluatedSweep, PreservesGeneratedPoseOrderAndProgress) {
  auto e = PoseEvaluator::create(box(), box()).value();
  auto expected = generate_sweep_pose_frames(SweepPreset::closing, 31).value();
  auto actual = e.evaluate_sweep(SweepPreset::closing, 31).value();
  ASSERT_EQ(actual.frames.size(), expected.size());
  EXPECT_DOUBLE_EQ(actual.frames.front().normalized_progress, 0);
  EXPECT_DOUBLE_EQ(actual.frames.back().normalized_progress, 1);
  for (std::size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(actual.frames[i].index, i);
    EXPECT_DOUBLE_EQ(actual.frames[i].normalized_progress, expected[i].normalized_progress);
    EXPECT_TRUE(same_pose(actual.frames[i].requested_pose, expected[i].pose));
    EXPECT_TRUE(same_pose(actual.frames[i].evaluation.requested_pose, expected[i].pose));
    if (i) {
      EXPECT_LE(actual.frames[i - 1].normalized_progress, actual.frames[i].normalized_progress);
    }
  }
}
TEST(EvaluatedSweep, BoundsFiniteUniqueSamplesAcrossMaximumSweep) {
  auto e = PoseEvaluator::create(box(), box()).value();
  auto result = e.evaluate_sweep(SweepPreset::closing, 61).value();
  std::size_t total = 0;
  for (const auto& frame : result.frames) {
    EXPECT_LE(frame.evaluation.contacts.size(), maximum_contact_samples);
    total += frame.evaluation.contacts.size();
    std::set<std::string> ids;
    for (const auto& contact : frame.evaluation.contacts) {
      EXPECT_TRUE(std::isfinite(contact.position_meters.x));
      EXPECT_TRUE(std::isfinite(contact.penetration_depth.value()));
      ids.insert(contact.stable_id);
    }
    EXPECT_EQ(ids.size(), frame.evaluation.contacts.size());
  }
  EXPECT_LE(total, std::size_t{1952});
}
TEST(EvaluatedSweep, IsDeterministic) {
  auto e = PoseEvaluator::create(box(), box()).value();
  auto a = e.evaluate_sweep(SweepPreset::protrusive, 31).value();
  auto b = e.evaluate_sweep(SweepPreset::protrusive, 31).value();
  ASSERT_EQ(a.frames.size(), b.frames.size());
  for (std::size_t i = 0; i < a.frames.size(); ++i) {
    EXPECT_EQ(a.frames[i].evaluation.classification, b.frames[i].evaluation.classification);
    EXPECT_EQ(a.frames[i].evaluation.contacts, b.frames[i].evaluation.contacts);
  }
  EXPECT_EQ(e.query_count(), 62);
}
TEST(SweepSummary, RejectsEmptyNoncontiguousAndNonmonotonicFrames) {
  EXPECT_FALSE(summarize_evaluated_sweep({}));
  auto frames = std::vector{summary_frame(0, 2, ContactClassification::separated, 0),
                            summary_frame(1, 2, ContactClassification::touching, 0)};
  frames[1].index = 3;
  auto bad_index = summarize_evaluated_sweep(frames);
  ASSERT_FALSE(bad_index);
  EXPECT_EQ(bad_index.errors()[0].field, "frames[1].index");
  frames[1].index = 1;
  frames[1].normalized_progress = -0.1;
  EXPECT_FALSE(summarize_evaluated_sweep(frames));
  frames[1].normalized_progress = std::numeric_limits<double>::quiet_NaN();
  EXPECT_FALSE(summarize_evaluated_sweep(frames));
}
TEST(SweepSummary, RejectsInconsistentClassification) {
  auto frame = summary_frame(0, 2, ContactClassification::separated, 0);
  frame.normalized_progress = 0;
  frame.evaluation.normalized_contact_count = 1;
  EXPECT_FALSE(summarize_evaluated_sweep({frame}));
}
TEST(SweepSummary, UsesEarliestMaximumAndFinalContact) {
  auto frames = std::vector{summary_frame(0, 3, ContactClassification::penetrating, .005),
                            summary_frame(1, 3, ContactClassification::penetrating, .002),
                            summary_frame(2, 3, ContactClassification::penetrating, .005)};
  auto summary = summarize_evaluated_sweep(frames).value();
  EXPECT_EQ(summary.first_contact_frame, 0);
  EXPECT_EQ(summary.last_contact_frame, 2);
  EXPECT_EQ(summary.maximum_penetration_frame, 0);
  EXPECT_TRUE(summary.contact_persists_through_final_frame);
}
TEST(SweepSummary, RepresentsNoContactIndexesAsAbsent) {
  auto frames = std::vector{summary_frame(0, 2, ContactClassification::separated, 0),
                            summary_frame(1, 2, ContactClassification::separated, 0)};
  auto summary = summarize_evaluated_sweep(frames).value();
  EXPECT_FALSE(summary.first_contact_frame);
  EXPECT_FALSE(summary.last_contact_frame);
  EXPECT_FALSE(summary.maximum_penetration_frame);
  EXPECT_FALSE(summary.contact_persists_through_final_frame);
}
TEST(SweepSummaryFixture, ExecutesAllGoldenCases) {
  std::ifstream file(OCCLUSION_SWEEP_SUMMARY_FIXTURE_SOURCE);
  nlohmann::json fixture;
  file >> fixture;
  ASSERT_EQ(fixture["schemaVersion"], "1.0.0");
  ASSERT_EQ(fixture["sourceBaselineCommit"], "46ffc9524e781fe2e8d8c269027434f422c2abf7");
  ASSERT_EQ(fixture["cases"].size(), 12);
  for (const auto& test_case : fixture["cases"]) {
    std::vector<EvaluatedSweepFrame> frames;
    const auto count = test_case["frames"].size();
    for (std::size_t i = 0; i < count; ++i) {
      const auto& input = test_case["frames"][i];
      const std::string state = input["classification"];
      const auto classification = state == "separated"  ? ContactClassification::separated
                                  : state == "touching" ? ContactClassification::touching
                                                        : ContactClassification::penetrating;
      frames.push_back(summary_frame(i, count, classification, input["penetrationDepthMeters"]));
    }
    const auto summary = summarize_evaluated_sweep(frames);
    ASSERT_TRUE(summary) << test_case["id"];
    const auto& expected = test_case["expected"];
    EXPECT_EQ(summary.value().total_frame_count, expected["totalFrameCount"]);
    EXPECT_EQ(summary.value().contact_frame_count, expected["contactFrameCount"]);
    EXPECT_DOUBLE_EQ(summary.value().maximum_penetration.value(),
                     expected["maximumPenetrationMeters"]);
    EXPECT_EQ(summary.value().contact_persists_through_final_frame,
              expected["contactPersistsThroughFinalFrame"]);
    if (expected["firstContactFrame"].is_null())
      EXPECT_FALSE(summary.value().first_contact_frame);
    else
      EXPECT_EQ(summary.value().first_contact_frame, expected["firstContactFrame"]);
    if (expected["maximumPenetrationFrame"].is_null())
      EXPECT_FALSE(summary.value().maximum_penetration_frame);
    else
      EXPECT_EQ(summary.value().maximum_penetration_frame, expected["maximumPenetrationFrame"]);
  }
}
TEST(SweepSummaryFixture, TrackedFixtureCopyIsByteExact) {
  EXPECT_EQ(read(OCCLUSION_SWEEP_SUMMARY_FIXTURE_SOURCE),
            read(OCCLUSION_SWEEP_SUMMARY_FIXTURE_COPY));
}
