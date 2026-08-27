#include "occlusion/core/domain.hpp"
#include <array>
#include <cmath>
#include <fstream>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
#include <string>

namespace occlusion::core {
namespace {
using Json = nlohmann::json;

Json read_fixture() {
  std::ifstream input(OCCLUSION_SWEEP_FIXTURE_PATH);
  if (!input)
    throw std::runtime_error("unable to open copied sweep fixture");
  return Json::parse(input);
}

void require(bool condition, const std::string& message) {
  if (!condition)
    throw std::runtime_error(message);
}

SweepPreset parse_preset(const std::string& value) {
  if (value == "closing")
    return SweepPreset::closing;
  if (value == "protrusive")
    return SweepPreset::protrusive;
  if (value == "left-lateral")
    return SweepPreset::left_lateral;
  if (value == "right-lateral")
    return SweepPreset::right_lateral;
  throw std::runtime_error("unsupported sweep preset: " + value);
}

MandibularPose parse_pose(const Json& value) {
  return {Meters{value.at("openingMeters").get<double>()},
          Meters{value.at("protrusionMeters").get<double>()},
          Meters{value.at("lateralMeters").get<double>()}};
}

void validate_fixture_shape(const Json& fixture) {
  require(fixture.is_object(), "fixture root must be an object");
  require(fixture.at("schemaVersion") == 1, "unsupported sweep fixture schema");
  require(fixture.at("legacyBehaviorVersion") == "mandibular-sweep-generation-v1",
          "unexpected legacy behavior version");
  require(fixture.at("workerProtocolVersion") == 4, "unexpected worker protocol version");
  require(fixture.at("sourceImplementation").is_string(), "missing source implementation");
  require(fixture.at("sourceBaselineCommit") == "39e206e48599333d2fa76947352e09be04c39dee",
          "unexpected source baseline");
  require(fixture.at("internalUnit") == "meters", "unexpected internal unit");
  require(fixture.at("poseRoundingPrecisionDecimalPlaces") == 6,
          "unexpected pose rounding precision");
  require(fixture.at("progressFormula") == "frameIndex / (frameCount - 1)",
          "unexpected progress formula");
  require(fixture.at("numericComparisonTolerance").is_number(), "missing tolerance");
  require(fixture.at("cases").is_array() && fixture.at("cases").size() == 20,
          "fixture must contain 20 cases");
  require(fixture.at("supportedPresets") ==
              Json::array({"closing", "protrusive", "left-lateral", "right-lateral"}),
          "preset metadata mismatch");
}

void expect_pose_near(const MandibularPose& actual, const MandibularPose& expected,
                      double tolerance, const std::string& case_id) {
  EXPECT_NEAR(actual.opening.value(), expected.opening.value(), tolerance) << case_id;
  EXPECT_NEAR(actual.protrusion.value(), expected.protrusion.value(), tolerance) << case_id;
  EXPECT_NEAR(actual.lateral_displacement.value(), expected.lateral_displacement.value(), tolerance)
      << case_id;
}
} // namespace

TEST(SweepGoldenParity, ExecutesEveryVersionedCaseExactlyOnce) {
  const auto fixture = read_fixture();
  ASSERT_NO_THROW(validate_fixture_shape(fixture));
  EXPECT_EQ(fixture.at("frameCountLimits").at("minimum"), minimum_sweep_frame_count);
  EXPECT_EQ(fixture.at("frameCountLimits").at("maximum"), maximum_sweep_frame_count);
  EXPECT_EQ(fixture.at("defaultFrameCount"), default_sweep_frame_count);
  const double tolerance = fixture.at("numericComparisonTolerance").get<double>();
  std::set<std::string> executed_ids;
  std::size_t total_frames = 0;
  for (const auto& fixture_case : fixture.at("cases")) {
    const std::string case_id = fixture_case.at("id").get<std::string>();
    SCOPED_TRACE(case_id);
    ASSERT_TRUE(executed_ids.insert(case_id).second) << case_id;
    const auto preset = parse_preset(fixture_case.at("preset").get<std::string>());
    const auto endpoints = sweep_endpoints(preset);
    expect_pose_near(endpoints.start, parse_pose(fixture_case.at("startPose")), tolerance, case_id);
    expect_pose_near(endpoints.end, parse_pose(fixture_case.at("endPose")), tolerance, case_id);
    const auto frame_count = fixture_case.at("frameCount").get<std::size_t>();
    const auto generated = generate_sweep_pose_frames(preset, frame_count);
    ASSERT_TRUE(generated) << case_id;
    ASSERT_EQ(generated.value().size(), fixture_case.at("frames").size()) << case_id;
    double previous_progress = -1.0;
    for (std::size_t index = 0; index < generated.value().size(); ++index) {
      const auto& actual = generated.value()[index];
      const auto& expected = fixture_case.at("frames").at(index);
      EXPECT_EQ(actual.index, index) << case_id;
      EXPECT_EQ(expected.at("frameIndex"), index) << case_id;
      EXPECT_NEAR(actual.normalized_progress, expected.at("progress").get<double>(), tolerance)
          << case_id;
      EXPECT_GT(actual.normalized_progress, previous_progress) << case_id;
      EXPECT_TRUE(validate(actual.pose)) << case_id;
      expect_pose_near(actual.pose, parse_pose(expected.at("expectedPose")), tolerance, case_id);
      const auto transform = mandibular_pose_to_transform(actual.pose);
      ASSERT_TRUE(transform) << case_id;
      const auto& translation = expected.at("expectedTranslationMeters");
      EXPECT_NEAR(transform.value().translation_meters.x, translation.at("x").get<double>(),
                  tolerance)
          << case_id;
      EXPECT_NEAR(transform.value().translation_meters.y, translation.at("y").get<double>(),
                  tolerance)
          << case_id;
      EXPECT_NEAR(transform.value().translation_meters.z, translation.at("z").get<double>(),
                  tolerance)
          << case_id;
      EXPECT_EQ(transform.value().rotation,
                (std::array<double, 9>{1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0}))
          << case_id;
      EXPECT_EQ(expected.at("expectedRotationQuaternion"),
                Json({{"x", 0}, {"y", 0}, {"z", 0}, {"w", 1}}))
          << case_id;
      previous_progress = actual.normalized_progress;
    }
    EXPECT_DOUBLE_EQ(generated.value().front().normalized_progress, 0.0) << case_id;
    EXPECT_DOUBLE_EQ(generated.value().back().normalized_progress, 1.0) << case_id;
    expect_pose_near(generated.value().front().pose, endpoints.start, 0.0, case_id);
    expect_pose_near(generated.value().back().pose, endpoints.end, 0.0, case_id);
    total_frames += generated.value().size();
  }
  EXPECT_EQ(executed_ids.size(), 20U);
  EXPECT_EQ(total_frames, 432U);
  EXPECT_FALSE(generate_sweep_pose_frames(SweepPreset::closing, 1));
  EXPECT_FALSE(generate_sweep_pose_frames(SweepPreset::closing, 62));
}

TEST(SweepGoldenParity, RejectsMalformedFixtureDeterministically) {
  auto malformed = read_fixture();
  malformed.erase("progressFormula");
  EXPECT_THROW(validate_fixture_shape(malformed), std::exception);
  EXPECT_THROW(validate_fixture_shape(malformed), std::exception);
  malformed = read_fixture();
  malformed["supportedPresets"][0] = "opening";
  EXPECT_THROW(validate_fixture_shape(malformed), std::runtime_error);
}
} // namespace occlusion::core
