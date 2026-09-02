#pragma once
#include "occlusion/collision/collision.hpp"
#include "occlusion/core/domain.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace occlusion::evaluation {
inline constexpr std::size_t maximum_contact_samples = 32;
inline constexpr double contact_deduplication_grid_meters = 1e-5;
inline constexpr int contact_quantization_decimal_places = 6;
inline constexpr double normal_unit_tolerance = 1e-9;
inline constexpr const char* normalization_algorithm_version = "1";

enum class MeasurementStatus { available, unavailable };
struct NormalizedContactSample final {
  std::string stable_id;
  occlusion::core::Vector3 position_meters;
  occlusion::core::Vector3 normal_fixed_to_moving;
  occlusion::core::Meters penetration_depth;
  occlusion::core::ContactClassification classification;
  [[nodiscard]] bool operator==(const NormalizedContactSample& other) const {
    return stable_id == other.stable_id && position_meters.x == other.position_meters.x &&
           position_meters.y == other.position_meters.y &&
           position_meters.z == other.position_meters.z &&
           normal_fixed_to_moving.x == other.normal_fixed_to_moving.x &&
           normal_fixed_to_moving.y == other.normal_fixed_to_moving.y &&
           normal_fixed_to_moving.z == other.normal_fixed_to_moving.z &&
           penetration_depth.value() == other.penetration_depth.value() &&
           classification == other.classification;
  }
};
struct PoseEvaluationResult final {
  occlusion::core::MandibularPose requested_pose;
  occlusion::core::RigidTransform applied_transform;
  occlusion::core::ContactClassification classification;
  MeasurementStatus measurement_status;
  std::optional<occlusion::core::Meters> clearance;
  occlusion::core::Meters penetration_depth;
  std::optional<bool> intersects;
  std::size_t normalized_contact_count;
  std::vector<NormalizedContactSample> contacts;
};
struct EvaluatedSweepFrame final {
  std::size_t index;
  double normalized_progress;
  occlusion::core::MandibularPose requested_pose;
  PoseEvaluationResult evaluation;
};
struct EvaluatedSweepSummary final {
  std::size_t total_frame_count;
  std::optional<std::size_t> first_contact_frame;
  std::optional<std::size_t> last_contact_frame;
  std::size_t contact_frame_count;
  occlusion::core::Meters maximum_penetration;
  std::optional<std::size_t> maximum_penetration_frame;
  bool contact_persists_through_final_frame;
};
struct EvaluatedSweepResult final {
  occlusion::core::SweepPreset preset;
  std::size_t requested_frame_count;
  occlusion::core::MandibularPose final_pose;
  std::vector<EvaluatedSweepFrame> frames;
  EvaluatedSweepSummary summary;
};
[[nodiscard]] occlusion::core::ValidationResult<EvaluatedSweepSummary>
summarize_evaluated_sweep(const std::vector<EvaluatedSweepFrame>& frames);
[[nodiscard]] occlusion::core::ValidationResult<std::vector<NormalizedContactSample>>
normalize_contacts(std::vector<occlusion::collision::ContactCandidate> candidates);

class PoseEvaluator final {
public:
  [[nodiscard]] static occlusion::core::ValidationResult<PoseEvaluator>
  create(occlusion::collision::TriangleMesh fixed, occlusion::collision::TriangleMesh moving);
  [[nodiscard]] occlusion::core::ValidationResult<PoseEvaluationResult>
  evaluate(occlusion::core::MandibularPose pose) const;
  [[nodiscard]] occlusion::core::ValidationResult<EvaluatedSweepResult>
  evaluate_sweep(occlusion::core::SweepPreset preset, std::size_t frame_count) const;
  [[nodiscard]] std::size_t compilation_count() const noexcept;
  [[nodiscard]] std::size_t query_count() const noexcept;

private:
  struct Implementation;
  explicit PoseEvaluator(std::shared_ptr<Implementation> implementation) noexcept;
  std::shared_ptr<Implementation> implementation_;
};
} // namespace occlusion::evaluation
