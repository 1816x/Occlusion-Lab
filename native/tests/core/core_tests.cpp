#include <concepts>
#include <limits>
#include <type_traits>

#include <gtest/gtest.h>

#include "occlusion/core/domain.hpp"

namespace occlusion::core {
namespace {

MandibularPose pose(double opening = 0.01) {
  return {Meters{opening}, Meters{0.002}, Meters{-0.001}};
}

EvaluationResult evaluation() { return {pose(), {}, true}; }

SweepFrame frame(std::size_t index = 0, double progress = 0.0) {
  return {index, progress, pose(), evaluation()};
}

TEST(Length, ConvertsMillimetersToMeters) {
  EXPECT_DOUBLE_EQ(to_meters(Millimeters{25.0}).value(), 0.025);
}
TEST(Length, ConvertsMetersToMillimeters) {
  EXPECT_DOUBLE_EQ(to_millimeters(Meters{0.025}).value(), 25.0);
}
TEST(Length, RoundTripsWithinExplicitTolerance) {
  constexpr double tolerance = 1e-12;
  EXPECT_NEAR(to_millimeters(to_meters(Millimeters{12.345})).value(), 12.345, tolerance);
}
TEST(Length, PreventsImplicitUnitMixing) {
  static_assert(!std::convertible_to<Meters, Millimeters>);
  static_assert(!std::convertible_to<Millimeters, Meters>);
  static_assert(!std::convertible_to<double, Meters>);
  SUCCEED();
}
TEST(PoseValidation, AcceptsFinitePose) { EXPECT_TRUE(validate(pose())); }
TEST(PoseValidation, RejectsNan) {
  EXPECT_FALSE(validate(pose(std::numeric_limits<double>::quiet_NaN())));
}
TEST(PoseValidation, RejectsPositiveInfinity) {
  EXPECT_FALSE(validate(pose(std::numeric_limits<double>::infinity())));
}
TEST(PoseValidation, RejectsNegativeInfinity) {
  EXPECT_FALSE(validate(pose(-std::numeric_limits<double>::infinity())));
}
TEST(PoseValidation, AcceptsInclusiveBoundaries) {
  EXPECT_TRUE(validate(MandibularPose{opening_limits.minimum, protrusion_limits.minimum,
                                      lateral_displacement_limits.minimum}));
  EXPECT_TRUE(validate(MandibularPose{opening_limits.maximum, protrusion_limits.maximum,
                                      lateral_displacement_limits.maximum}));
}
TEST(PoseValidation, RejectsEachOutOfRangeFieldWithoutClamping) {
  const auto opening = validate(MandibularPose{Meters{-0.001}, Meters{0.0}, Meters{0.0}});
  ASSERT_FALSE(opening);
  EXPECT_EQ(opening.errors().front().code, ValidationCode::invalid_range);
  EXPECT_EQ(opening.errors().front().field, "openingMeters");
  const auto protrusion = validate(MandibularPose{Meters{0.0}, Meters{0.051}, Meters{0.0}});
  ASSERT_FALSE(protrusion);
  EXPECT_EQ(protrusion.errors().front().field, "protrusionMeters");
  const auto lateral = validate(MandibularPose{Meters{0.0}, Meters{0.0}, Meters{-0.051}});
  ASSERT_FALSE(lateral);
  EXPECT_EQ(lateral.errors().front().field, "lateralMeters");
}
TEST(PoseTransform, MapsLegacyTranslationAndIdentityRotation) {
  const auto result = mandibular_pose_to_transform({Meters{0.123}, Meters{0.034}, Meters{-0.012}});
  ASSERT_TRUE(result);
  EXPECT_DOUBLE_EQ(result.value().translation_meters.x, -0.012);
  EXPECT_DOUBLE_EQ(result.value().translation_meters.y, 0.16 - 0.123);
  EXPECT_DOUBLE_EQ(result.value().translation_meters.z, 0.034);
  EXPECT_EQ(result.value().rotation,
            (std::array<double, 9>{1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0}));
  EXPECT_FALSE(mandibular_pose_to_transform({Meters{0.251}, Meters{0.0}, Meters{0.0}}));
}
TEST(TransformValidation, AcceptsRigidTransform) {
  const RigidTransform transform{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0}};
  EXPECT_TRUE(validate(transform));
}
TEST(TransformValidation, RejectsInvalidRotation) {
  const RigidTransform transform{{0.0, 0.0, 0.0}, {2.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0}};
  const auto result = validate(transform);
  ASSERT_FALSE(result);
  EXPECT_EQ(result.errors().front().code, ValidationCode::invalid_rotation);
}
TEST(TransformValidation, RejectsNonFiniteTranslation) {
  const RigidTransform transform{{0.0, std::numeric_limits<double>::infinity(), 0.0},
                                 {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0}};
  EXPECT_FALSE(validate(transform));
}
TEST(ContactContract, PreservesClassification) {
  const ContactSample sample{
      {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, Meters{-0.0001}, ContactClassification::penetration};
  const auto result = validate(sample);
  ASSERT_TRUE(result);
  EXPECT_EQ(result.value().classification, ContactClassification::penetration);
}
TEST(SweepFrameValidation, AcceptsValidAndRejectsOutOfRangeProgress) {
  EXPECT_TRUE(validate(frame(0, 0.5)));
  EXPECT_FALSE(validate(frame(0, 1.01)));
}
TEST(SweepSummaryValidation, AcceptsConsistentSummary) {
  EXPECT_TRUE(validate(SweepSummary{SweepPreset::opening, 2, {frame(0, 0.0), frame(1, 1.0)}, 0}));
}
TEST(SweepSummaryValidation, RejectsInconsistentCountsAndIndices) {
  EXPECT_FALSE(validate(SweepSummary{SweepPreset::opening, 2, {frame(1, 0.0)}, 2}));
}

} // namespace
} // namespace occlusion::core
