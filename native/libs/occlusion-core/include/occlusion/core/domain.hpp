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
struct PoseLimits final {
  Meters minimum;
  Meters maximum;
};
inline constexpr PoseLimits opening_limits{Meters{0.0}, Meters{0.25}};
inline constexpr PoseLimits protrusion_limits{Meters{0.0}, Meters{0.05}};
inline constexpr PoseLimits lateral_displacement_limits{Meters{-0.05}, Meters{0.05}};
inline constexpr Meters closed_mandible_translation_y{0.16};
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
enum class SweepPreset { closing, protrusive, left_lateral, right_lateral };
inline constexpr std::size_t minimum_sweep_frame_count = 2;
inline constexpr std::size_t maximum_sweep_frame_count = 61;
inline constexpr std::size_t default_sweep_frame_count = 31;
struct SweepEndpoints final {
  MandibularPose start;
  MandibularPose end;
};
struct SweepPoseFrame final {
  std::size_t index;
  double normalized_progress;
  MandibularPose pose;
};
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
[[nodiscard]] SweepEndpoints sweep_endpoints(SweepPreset preset);
[[nodiscard]] ValidationResult<std::vector<SweepPoseFrame>>
generate_sweep_pose_frames(SweepPreset preset, std::size_t frame_count);
[[nodiscard]] ValidationResult<RigidTransform> mandibular_pose_to_transform(MandibularPose pose);
[[nodiscard]] ValidationResult<RigidTransform> validate(RigidTransform transform);
[[nodiscard]] ValidationResult<ContactSample> validate(ContactSample sample);
[[nodiscard]] ValidationResult<EvaluationResult> validate(EvaluationResult result);
[[nodiscard]] ValidationResult<SweepFrame> validate(SweepFrame frame);
[[nodiscard]] ValidationResult<SweepSummary> validate(SweepSummary summary);
} // namespace occlusion::core
