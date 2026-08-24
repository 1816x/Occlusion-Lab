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
} // namespace
ValidationResult<MandibularPose> validate(MandibularPose pose) {
  std::vector<ValidationError> errors;
  require_finite(pose.opening.value(), "opening", errors);
  require_finite(pose.protrusion.value(), "protrusion", errors);
  require_finite(pose.lateral_displacement.value(), "lateral_displacement", errors);
  return finish(std::move(pose), std::move(errors));
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
