#include <cmath>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "occlusion/core/domain.hpp"

#ifndef OCCLUSION_POSE_FIXTURE_PATH
#error "OCCLUSION_POSE_FIXTURE_PATH must identify the copied test fixture"
#endif

namespace occlusion::core {
namespace {
using Json = nlohmann::json;

double required_number(const Json& object, const char* key) {
  if (!object.contains(key) || !object.at(key).is_number())
    throw std::runtime_error(std::string{"missing or malformed number: "} + key);
  const auto value = object.at(key).get<double>();
  if (!std::isfinite(value))
    throw std::runtime_error(std::string{"non-finite number: "} + key);
  return value;
}
std::string required_string(const Json& object, const char* key) {
  if (!object.contains(key) || !object.at(key).is_string())
    throw std::runtime_error(std::string{"missing or malformed string: "} + key);
  return object.at(key).get<std::string>();
}
const Json& required_array(const Json& object, const char* key) {
  if (!object.contains(key) || !object.at(key).is_array())
    throw std::runtime_error(std::string{"missing or malformed array: "} + key);
  return object.at(key);
}
MandibularPose read_pose(const Json& value) {
  if (!value.is_object())
    throw std::runtime_error("malformed inputPose");
  return {Meters{required_number(value, "openingMeters")},
          Meters{required_number(value, "protrusionMeters")},
          Meters{required_number(value, "lateralMeters")}};
}
void validate_fixture_shape(const Json& root) {
  if (!root.is_object() || required_number(root, "schemaVersion") != 1.0)
    throw std::runtime_error("unsupported schemaVersion");
  for (const auto* key :
       {"legacyBehaviorVersion", "sourceImplementation", "sourceBaselineCommit", "internalUnit"})
    (void)required_string(root, key);
  (void)required_number(root, "workerProtocolVersion");
  (void)required_number(root, "closedMandibleTranslationYMeters");
  (void)required_number(root, "numericComparisonTolerance");
  if (!root.contains("poseLimits") || !root.at("poseLimits").is_object())
    throw std::runtime_error("missing or malformed poseLimits");
  (void)required_array(root, "validCases");
  (void)required_array(root, "invalidCases");
}
Json load_fixture() {
  std::ifstream stream{OCCLUSION_POSE_FIXTURE_PATH};
  if (!stream)
    throw std::runtime_error("unable to open copied pose fixture");
  return Json::parse(stream);
}
void expect_limit(const Json& limits, const char* key, PoseLimits expected) {
  ASSERT_TRUE(limits.contains(key));
  EXPECT_DOUBLE_EQ(required_number(limits.at(key), "min"), expected.minimum.value());
  EXPECT_DOUBLE_EQ(required_number(limits.at(key), "max"), expected.maximum.value());
}

TEST(PoseGoldenParity, ExecutesEveryVersionedCaseExactlyOnce) {
  const auto root = load_fixture();
  ASSERT_NO_THROW(validate_fixture_shape(root));
  const auto tolerance = required_number(root, "numericComparisonTolerance");
  expect_limit(root.at("poseLimits"), "openingMeters", opening_limits);
  expect_limit(root.at("poseLimits"), "protrusionMeters", protrusion_limits);
  expect_limit(root.at("poseLimits"), "lateralMeters", lateral_displacement_limits);
  EXPECT_DOUBLE_EQ(required_number(root, "closedMandibleTranslationYMeters"),
                   closed_mandible_translation_y.value());

  std::set<std::string> executed_ids;
  std::size_t executed = 0;
  for (const auto& fixture_case : required_array(root, "validCases")) {
    const auto id = required_string(fixture_case, "id");
    SCOPED_TRACE(id);
    ASSERT_TRUE(executed_ids.insert(id).second);
    ASSERT_TRUE(fixture_case.at("expectedAcceptance").is_boolean());
    ASSERT_TRUE(fixture_case.at("expectedAcceptance").get<bool>());
    const auto result = mandibular_pose_to_transform(read_pose(fixture_case.at("inputPose")));
    ASSERT_TRUE(result);
    const auto& expected = fixture_case.at("expectedTranslationMeters");
    EXPECT_NEAR(result.value().translation_meters.x, required_number(expected, "x"), tolerance);
    EXPECT_NEAR(result.value().translation_meters.y, required_number(expected, "y"), tolerance);
    EXPECT_NEAR(result.value().translation_meters.z, required_number(expected, "z"), tolerance);
    const auto& quaternion = fixture_case.at("expectedRotationQuaternion");
    EXPECT_DOUBLE_EQ(required_number(quaternion, "x"), 0.0);
    EXPECT_DOUBLE_EQ(required_number(quaternion, "y"), 0.0);
    EXPECT_DOUBLE_EQ(required_number(quaternion, "z"), 0.0);
    EXPECT_DOUBLE_EQ(std::abs(required_number(quaternion, "w")), 1.0);
    EXPECT_EQ(result.value().rotation,
              (std::array<double, 9>{1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0}));
    ++executed;
  }
  for (const auto& fixture_case : required_array(root, "invalidCases")) {
    const auto id = required_string(fixture_case, "id");
    SCOPED_TRACE(id);
    ASSERT_TRUE(executed_ids.insert(id).second);
    ASSERT_TRUE(fixture_case.at("expectedAcceptance").is_boolean());
    ASSERT_FALSE(fixture_case.at("expectedAcceptance").get<bool>());
    const auto result = validate(read_pose(fixture_case.at("inputPose")));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.errors().size(), 1U);
    EXPECT_EQ(result.errors().front().code, ValidationCode::invalid_range);
    EXPECT_EQ(result.errors().front().field,
              required_string(fixture_case, "expectedValidationField"));
    EXPECT_EQ(required_string(fixture_case, "expectedErrorCategory"), "invalid_range");
    EXPECT_FALSE(mandibular_pose_to_transform(read_pose(fixture_case.at("inputPose"))));
    ++executed;
  }
  EXPECT_EQ(executed, root.at("validCases").size() + root.at("invalidCases").size());
  EXPECT_EQ(executed_ids.size(), executed);
}

TEST(PoseGoldenParity, RejectsMalformedFixtureDeterministically) {
  const Json malformed = {
      {"schemaVersion", 1}, {"validCases", Json::array()}, {"invalidCases", Json::array()}};
  EXPECT_THROW(validate_fixture_shape(malformed), std::runtime_error);
  Json wrong_schema = load_fixture();
  wrong_schema["schemaVersion"] = 2;
  EXPECT_THROW(validate_fixture_shape(wrong_schema), std::runtime_error);
}
} // namespace
} // namespace occlusion::core
