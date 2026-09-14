#include "occlusion/evaluation/evaluation.hpp"
#include <atomic>
#include <utility>

namespace occlusion::evaluation {
using occlusion::core::ValidationResult;
struct PoseEvaluator::Implementation final {
  occlusion::collision::CollisionEngine engine;
  occlusion::collision::CollisionModel fixed;
  occlusion::collision::CollisionModel moving;
  std::atomic_size_t query_count{0};
  Implementation(occlusion::collision::CollisionModel fixed_model,
                 occlusion::collision::CollisionModel moving_model)
      : fixed(std::move(fixed_model)), moving(std::move(moving_model)) {}
};
PoseEvaluator::PoseEvaluator(std::shared_ptr<Implementation> implementation) noexcept
    : implementation_(std::move(implementation)) {}
ValidationResult<PoseEvaluator> PoseEvaluator::create(occlusion::collision::TriangleMesh fixed,
                                                      occlusion::collision::TriangleMesh moving) {
  occlusion::collision::CollisionEngine engine;
  auto fixed_model = engine.compile(std::move(fixed));
  if (!fixed_model) {
    auto errors = fixed_model.errors();
    for (auto& error : errors)
      error.field = "fixed." + error.field;
    return ValidationResult<PoseEvaluator>::failure(std::move(errors));
  }
  auto moving_model = engine.compile(std::move(moving));
  if (!moving_model) {
    auto errors = moving_model.errors();
    for (auto& error : errors)
      error.field = "moving." + error.field;
    return ValidationResult<PoseEvaluator>::failure(std::move(errors));
  }
  return ValidationResult<PoseEvaluator>::success(
      PoseEvaluator{std::make_shared<Implementation>(fixed_model.value(), moving_model.value())});
}
ValidationResult<PoseEvaluationResult>
PoseEvaluator::evaluate(occlusion::core::MandibularPose pose) const {
  const auto valid_pose = occlusion::core::validate(pose);
  if (!valid_pose)
    return ValidationResult<PoseEvaluationResult>::failure(valid_pose.errors());
  const auto transform = occlusion::core::mandibular_pose_to_transform(pose);
  if (!transform)
    return ValidationResult<PoseEvaluationResult>::failure(transform.errors());
  implementation_->query_count.fetch_add(1, std::memory_order_relaxed);
  const auto collision = implementation_->engine.query(implementation_->fixed,
                                                       implementation_->moving, transform.value());
  if (!collision)
    return ValidationResult<PoseEvaluationResult>::failure(collision.errors());
  auto contacts = normalize_contacts(collision.value().candidates);
  if (!contacts)
    return ValidationResult<PoseEvaluationResult>::failure(contacts.errors());
  std::optional<occlusion::core::Meters> clearance;
  if (collision.value().classification == occlusion::core::ContactClassification::separated)
    clearance = collision.value().clearance;
  auto samples = contacts.value();
  return ValidationResult<PoseEvaluationResult>::success(
      {pose, transform.value(), collision.value().classification, MeasurementStatus::available,
       clearance, collision.value().penetration_depth, collision.value().intersects, samples.size(),
       std::move(samples)});
}
ValidationResult<EvaluatedSweepResult>
PoseEvaluator::evaluate_sweep(occlusion::core::SweepPreset preset, std::size_t frame_count) const {
  const auto generated = occlusion::core::generate_sweep_pose_frames(preset, frame_count);
  if (!generated)
    return ValidationResult<EvaluatedSweepResult>::failure(generated.errors());

  std::vector<EvaluatedSweepFrame> frames;
  frames.reserve(generated.value().size());
  std::optional<std::size_t> first_contact;
  std::optional<std::size_t> last_contact;
  std::size_t contact_count = 0;
  occlusion::core::Meters maximum_penetration{0.0};
  std::size_t maximum_penetration_frame = 0;

  for (const auto& generated_frame : generated.value()) {
    auto evaluation = evaluate(generated_frame.pose);
    if (!evaluation) {
      auto errors = evaluation.errors();
      for (auto& error : errors)
        error.field = "frames[" + std::to_string(generated_frame.index) + "]." + error.field;
      return ValidationResult<EvaluatedSweepResult>::failure(std::move(errors));
    }
    auto value = evaluation.value();
    if (value.normalized_contact_count > 0) {
      if (!first_contact)
        first_contact = generated_frame.index;
      last_contact = generated_frame.index;
      ++contact_count;
    }
    if (value.penetration_depth.value() > maximum_penetration.value()) {
      maximum_penetration = value.penetration_depth;
      maximum_penetration_frame = generated_frame.index;
    }
    frames.push_back(
        {generated_frame.index, generated_frame.normalized_progress, std::move(value)});
  }

  const bool persists = last_contact && *last_contact + 1 == frames.size();
  EvaluatedSweepSummary summary{frames.size(), first_contact,       last_contact,
                                contact_count, maximum_penetration, maximum_penetration_frame,
                                persists};
  const auto final_pose = frames.back().evaluation.requested_pose;
  return ValidationResult<EvaluatedSweepResult>::success(
      {preset, frame_count, final_pose, std::move(frames), std::move(summary)});
}
std::size_t PoseEvaluator::compilation_count() const noexcept { return 2; }
std::size_t PoseEvaluator::query_count() const noexcept {
  return implementation_->query_count.load(std::memory_order_relaxed);
}
} // namespace occlusion::evaluation
