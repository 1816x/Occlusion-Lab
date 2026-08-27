#include "occlusion/core/domain.hpp"
#include <Eigen/Core>
#include <Eigen/LU>
#include <cmath>
#include <string>
#include <utility>
namespace occlusion::core {
namespace {
void require_finite(double value, std::string field, std::vector<ValidationError>& errors) {
  if (!std::isfinite(value))
    errors.push_back({ValidationCode::non_finite, std::move(field), "value must be finite"});
}
void require_finite(Vector3 value, const std::string& field, std::vector<ValidationError>& errors) {
  require_finite(value.x, field + ".x", errors);
  require_finite(value.y, field + ".y", errors);
  require_finite(value.z, field + ".z", errors);
}
template <typename T> ValidationResult<T> finish(T value, std::vector<ValidationError> errors) {
  if (!errors.empty())
    return ValidationResult<T>::failure(std::move(errors));
  return ValidationResult<T>::success(std::move(value));
}
void append_errors(const std::vector<ValidationError>& source,
                   std::vector<ValidationError>& destination) {
  destination.insert(destination.end(), source.begin(), source.end());
}
double quantize_pose_component(double value) {
  return std::round(value * 1'000'000.0) / 1'000'000.0;
}
} // namespace
ValidationResult<MandibularPose> validate(MandibularPose pose) {
  std::vector<ValidationError> errors;
  require_finite(pose.opening.value(), "openingMeters", errors);
  require_finite(pose.protrusion.value(), "protrusionMeters", errors);
  require_finite(pose.lateral_displacement.value(), "lateralMeters", errors);
  const auto require_range = [&errors](Meters value, PoseLimits limits, const char* field) {
    if (std::isfinite(value.value()) &&
        (value.value() < limits.minimum.value() || value.value() > limits.maximum.value()))
      errors.push_back(
          {ValidationCode::invalid_range, field, "value is outside the inclusive pose limits"});
  };
  require_range(pose.opening, opening_limits, "openingMeters");
  require_range(pose.protrusion, protrusion_limits, "protrusionMeters");
  require_range(pose.lateral_displacement, lateral_displacement_limits, "lateralMeters");
  return finish(std::move(pose), std::move(errors));
}
SweepEndpoints sweep_endpoints(SweepPreset preset) {
  constexpr MandibularPose contact{Meters{0.0}, Meters{0.0}, Meters{0.0}};
  switch (preset) {
  case SweepPreset::closing:
    return {{opening_limits.maximum, Meters{0.0}, Meters{0.0}}, contact};
  case SweepPreset::protrusive:
    return {contact, {Meters{0.0}, protrusion_limits.maximum, Meters{0.0}}};
  case SweepPreset::left_lateral:
    return {contact, {Meters{0.0}, Meters{0.0}, lateral_displacement_limits.minimum}};
  case SweepPreset::right_lateral:
    return {contact, {Meters{0.0}, Meters{0.0}, lateral_displacement_limits.maximum}};
  }
  return {contact, contact};
}
ValidationResult<std::vector<SweepPoseFrame>> generate_sweep_pose_frames(SweepPreset preset,
                                                                         std::size_t frame_count) {
  if (frame_count < minimum_sweep_frame_count || frame_count > maximum_sweep_frame_count)
    return ValidationResult<std::vector<SweepPoseFrame>>::failure(
        {{ValidationCode::invalid_range, "frame_count", "frame count must be in [2, 61]"}});
  const auto endpoints = sweep_endpoints(preset);
  std::vector<SweepPoseFrame> frames;
  frames.reserve(frame_count);
  for (std::size_t index = 0; index < frame_count; ++index) {
    const double progress = static_cast<double>(index) / static_cast<double>(frame_count - 1);
    const auto interpolate = [progress](Meters start, Meters end) {
      return Meters{
          quantize_pose_component(start.value() + (end.value() - start.value()) * progress)};
    };
    MandibularPose pose{
        interpolate(endpoints.start.opening, endpoints.end.opening),
        interpolate(endpoints.start.protrusion, endpoints.end.protrusion),
        interpolate(endpoints.start.lateral_displacement, endpoints.end.lateral_displacement)};
    if (index == 0)
      pose = endpoints.start;
    else if (index + 1 == frame_count)
      pose = endpoints.end;
    const auto validated = validate(pose);
    if (!validated)
      return ValidationResult<std::vector<SweepPoseFrame>>::failure(validated.errors());
    frames.push_back({index, progress, pose});
  }
  return ValidationResult<std::vector<SweepPoseFrame>>::success(std::move(frames));
}
ValidationResult<RigidTransform> mandibular_pose_to_transform(MandibularPose pose) {
  const auto validated = validate(pose);
  if (!validated)
    return ValidationResult<RigidTransform>::failure(validated.errors());
  return ValidationResult<RigidTransform>::success(
      {{pose.lateral_displacement.value(),
        closed_mandible_translation_y.value() - pose.opening.value(), pose.protrusion.value()},
       {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0}});
}
ValidationResult<RigidTransform> validate(RigidTransform transform) {
  std::vector<ValidationError> errors;
  require_finite(transform.translation_meters, "translation_meters", errors);
  for (std::size_t i = 0; i < transform.rotation.size(); ++i)
    require_finite(transform.rotation[i], "rotation[" + std::to_string(i) + "]", errors);
  if (errors.empty()) {
    const Eigen::Map<const Eigen::Matrix<double, 3, 3, Eigen::RowMajor>> rotation(
        transform.rotation.data());
    constexpr double tolerance = 1e-12;
    if (!(rotation.transpose() * rotation).isApprox(Eigen::Matrix3d::Identity(), tolerance) ||
        std::abs(rotation.determinant() - 1.0) > tolerance)
      errors.push_back(
          {ValidationCode::invalid_rotation, "rotation", "rotation must be orthonormal"});
  }
  return finish(std::move(transform), std::move(errors));
}
ValidationResult<ContactSample> validate(ContactSample sample) {
  std::vector<ValidationError> errors;
  require_finite(sample.position_meters, "position_meters", errors);
  require_finite(sample.normal, "normal", errors);
  require_finite(sample.signed_distance.value(), "signed_distance", errors);
  return finish(std::move(sample), std::move(errors));
}
ValidationResult<EvaluationResult> validate(EvaluationResult result) {
  std::vector<ValidationError> errors;
  append_errors(validate(result.pose).errors(), errors);
  for (const auto& c : result.contacts)
    append_errors(validate(c).errors(), errors);
  return finish(std::move(result), std::move(errors));
}
ValidationResult<SweepFrame> validate(SweepFrame frame) {
  std::vector<ValidationError> errors;
  require_finite(frame.normalized_progress, "normalized_progress", errors);
  if (std::isfinite(frame.normalized_progress) &&
      (frame.normalized_progress < 0.0 || frame.normalized_progress > 1.0))
    errors.push_back({ValidationCode::invalid_range, "normalized_progress",
                      "normalized progress must be in [0, 1]"});
  append_errors(validate(frame.pose).errors(), errors);
  append_errors(validate(frame.evaluation).errors(), errors);
  return finish(std::move(frame), std::move(errors));
}
ValidationResult<SweepSummary> validate(SweepSummary summary) {
  std::vector<ValidationError> errors;
  if (summary.frames.size() != summary.requested_frame_count ||
      summary.contact_frame_count > summary.frames.size())
    errors.push_back({ValidationCode::inconsistent_summary, "frames",
                      "summary counts must agree with the frame collection"});
  for (std::size_t i = 0; i < summary.frames.size(); ++i) {
    if (summary.frames[i].index != i)
      errors.push_back({ValidationCode::inconsistent_summary, "frames.index",
                        "frame indices must be contiguous"});
    append_errors(validate(summary.frames[i]).errors(), errors);
  }
  return finish(std::move(summary), std::move(errors));
}
} // namespace occlusion::core
