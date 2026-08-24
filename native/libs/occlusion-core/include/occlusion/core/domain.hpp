#pragma once
#include "occlusion/core/length.hpp"
#include "occlusion/core/validation.hpp"
#include <array>
#include <cstddef>
#include <vector>
namespace occlusion::core {
struct Vector3 final {
  double x;
  double y;
  double z;
};
struct MandibularPose final {
  Meters opening;
  Meters protrusion;
  Meters lateral_displacement;
};
struct RigidTransform final {
  Vector3 translation_meters;
  std::array<double, 9> rotation;
};
enum class ContactClassification { none, near_contact, contact, penetration };
struct ContactSample final {
  Vector3 position_meters;
  Vector3 normal;
  Meters signed_distance;
  ContactClassification classification;
};
struct EvaluationResult final {
  MandibularPose pose;
  std::vector<ContactSample> contacts;
  bool valid;
};
enum class SweepPreset { opening, protrusive, left_lateral, right_lateral };
struct SweepFrame final {
  std::size_t index;
  double normalized_progress;
  MandibularPose pose;
  EvaluationResult evaluation;
};
struct SweepSummary final {
  SweepPreset preset;
  std::size_t requested_frame_count;
  std::vector<SweepFrame> frames;
  std::size_t contact_frame_count;
};
[[nodiscard]] ValidationResult<MandibularPose> validate(MandibularPose pose);
[[nodiscard]] ValidationResult<RigidTransform> validate(RigidTransform transform);
[[nodiscard]] ValidationResult<ContactSample> validate(ContactSample sample);
[[nodiscard]] ValidationResult<EvaluationResult> validate(EvaluationResult result);
[[nodiscard]] ValidationResult<SweepFrame> validate(SweepFrame frame);
[[nodiscard]] ValidationResult<SweepSummary> validate(SweepSummary summary);
} // namespace occlusion::core
