#include "occlusion/evaluation/evaluation.hpp"
#include <cmath>
#include <string>
#include <utility>

namespace occlusion::evaluation {
namespace {
using occlusion::core::ContactClassification;
using occlusion::core::ValidationCode;
using occlusion::core::ValidationError;

void error(std::vector<ValidationError>& errors, std::string field, std::string message) {
  errors.push_back({ValidationCode::inconsistent_summary, std::move(field), std::move(message)});
}
bool same_pose(const occlusion::core::MandibularPose& a, const occlusion::core::MandibularPose& b) {
  return a.opening.value() == b.opening.value() && a.protrusion.value() == b.protrusion.value() &&
         a.lateral_displacement.value() == b.lateral_displacement.value();
}
} // namespace

occlusion::core::ValidationResult<EvaluatedSweepSummary>
summarize_evaluated_sweep(const std::vector<EvaluatedSweepFrame>& frames) {
  std::vector<ValidationError> errors;
  if (frames.empty())
    error(errors, "frames", "an evaluated sweep must contain frames");
  double previous_progress = -1.0;
  for (std::size_t position = 0; position < frames.size(); ++position) {
    const auto& frame = frames[position];
    const auto prefix = "frames[" + std::to_string(position) + "].";
    if (frame.index != position)
      error(errors, prefix + "index", "frame indices must be contiguous and ascending");
    if (!std::isfinite(frame.normalized_progress))
      errors.push_back({ValidationCode::non_finite, prefix + "normalized_progress",
                        "normalized progress must be finite"});
    else if (frame.normalized_progress < 0.0 || frame.normalized_progress > 1.0)
      errors.push_back({ValidationCode::invalid_range, prefix + "normalized_progress",
                        "normalized progress must be in [0, 1]"});
    else if (frame.normalized_progress < previous_progress)
      error(errors, prefix + "normalized_progress", "normalized progress must be monotonic");
    previous_progress = frame.normalized_progress;
    const auto& evaluation = frame.evaluation;
    if (!same_pose(frame.requested_pose, evaluation.requested_pose))
      error(errors, prefix + "evaluation.requested_pose", "requested poses must agree");
    if (!std::isfinite(evaluation.penetration_depth.value()) ||
        evaluation.penetration_depth.value() < 0.0)
      error(errors, prefix + "evaluation.penetration_depth",
            "penetration must be finite and nonnegative");
    if (evaluation.normalized_contact_count != evaluation.contacts.size() ||
        evaluation.contacts.size() > maximum_contact_samples)
      error(errors, prefix + "evaluation.contacts", "published contact count is inconsistent");
    const bool has_contacts = !evaluation.contacts.empty();
    if ((evaluation.classification == ContactClassification::separated &&
         (has_contacts || !evaluation.clearance || evaluation.penetration_depth.value() != 0.0)) ||
        (evaluation.classification == ContactClassification::touching &&
         (!has_contacts || evaluation.clearance ||
          evaluation.penetration_depth.value() >
              occlusion::collision::contact_tolerance.value())) ||
        (evaluation.classification == ContactClassification::penetrating &&
         (!has_contacts || evaluation.clearance ||
          evaluation.penetration_depth.value() <= occlusion::collision::contact_tolerance.value())))
      error(errors, prefix + "evaluation.classification",
            "classification and measurements disagree");
  }
  if (!errors.empty())
    return occlusion::core::ValidationResult<EvaluatedSweepSummary>::failure(std::move(errors));

  EvaluatedSweepSummary summary{
      frames.size(), std::nullopt, std::nullopt, 0, occlusion::core::Meters{0.0},
      std::nullopt,  false};
  for (const auto& frame : frames) {
    if (!frame.evaluation.contacts.empty()) {
      if (!summary.first_contact_frame)
        summary.first_contact_frame = frame.index;
      summary.last_contact_frame = frame.index;
      ++summary.contact_frame_count;
    }
    if (frame.evaluation.penetration_depth.value() > summary.maximum_penetration.value()) {
      summary.maximum_penetration = frame.evaluation.penetration_depth;
      summary.maximum_penetration_frame = frame.index;
    }
  }
  summary.contact_persists_through_final_frame =
      summary.last_contact_frame && *summary.last_contact_frame == frames.back().index;
  return occlusion::core::ValidationResult<EvaluatedSweepSummary>::success(std::move(summary));
}
} // namespace occlusion::evaluation
