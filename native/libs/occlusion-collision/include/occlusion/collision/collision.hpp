#pragma once
#include "occlusion/core/domain.hpp"
#include <cstdint>
#include <memory>
#include <vector>

namespace occlusion::collision {
inline constexpr occlusion::core::Meters contact_tolerance{1e-6};
struct Triangle final {
  std::uint32_t a;
  std::uint32_t b;
  std::uint32_t c;
};
struct TriangleMesh final {
  std::vector<occlusion::core::Vector3> vertices_meters;
  std::vector<Triangle> triangles;
};
// Engine-neutral raw candidate. The normal is canonical from fixed maxillary
// geometry toward moving mandibular geometry in world coordinates.
struct ContactCandidate final {
  occlusion::core::Vector3 position_meters;
  occlusion::core::Vector3 normal_fixed_to_moving;
  occlusion::core::Meters signed_distance;
  occlusion::core::Meters penetration_depth;
  occlusion::core::ContactClassification classification;
};
struct SinglePoseCollisionResult final {
  occlusion::core::ContactClassification classification;
  occlusion::core::Meters clearance;
  occlusion::core::Meters penetration_depth;
  bool intersects;
  std::vector<ContactCandidate> candidates;
};

class CollisionModel final {
public:
  CollisionModel(const CollisionModel&) noexcept = default;
  CollisionModel(CollisionModel&&) noexcept = default;
  CollisionModel& operator=(const CollisionModel&) noexcept = default;
  CollisionModel& operator=(CollisionModel&&) noexcept = default;
  ~CollisionModel() = default;
  [[nodiscard]] std::size_t vertex_count() const noexcept;
  [[nodiscard]] std::size_t triangle_count() const noexcept;

private:
  struct Implementation;
  explicit CollisionModel(std::shared_ptr<const Implementation> implementation) noexcept;
  std::shared_ptr<const Implementation> implementation_;
  friend class CollisionEngine;
};

// Compilation owns an immutable BVH. Copies safely share it. Concurrent queries
// are safe: the engine is stateless and every query uses local request objects.
class CollisionEngine final {
public:
  [[nodiscard]] occlusion::core::ValidationResult<CollisionModel> compile(TriangleMesh mesh) const;
  [[nodiscard]] occlusion::core::ValidationResult<SinglePoseCollisionResult>
  query(const CollisionModel& fixed, const CollisionModel& moving,
        const occlusion::core::RigidTransform& moving_transform) const;
};
} // namespace occlusion::collision
