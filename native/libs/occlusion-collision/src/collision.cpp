#include "occlusion/collision/collision.hpp"
#include <algorithm>
#include <cmath>
#include <fcl/fcl.h>
#include <limits>
#include <string>
#include <utility>

namespace occlusion::collision {
using occlusion::core::ContactClassification;
using occlusion::core::Meters;
using occlusion::core::ValidationCode;
using occlusion::core::ValidationError;
using occlusion::core::ValidationResult;
using Bvh = fcl::BVHModel<fcl::OBBRSSd>;
struct CollisionModel::Implementation final {
  std::shared_ptr<Bvh> geometry;
  std::size_t vertices;
  std::size_t triangles;
};
CollisionModel::CollisionModel(std::shared_ptr<const Implementation> implementation) noexcept
    : implementation_(std::move(implementation)) {}
std::size_t CollisionModel::vertex_count() const noexcept { return implementation_->vertices; }
std::size_t CollisionModel::triangle_count() const noexcept { return implementation_->triangles; }
namespace {
ValidationError error(ValidationCode code, std::string field, std::string message) {
  return {code, std::move(field), std::move(message)};
}
fcl::Transform3d transform_of(const occlusion::core::RigidTransform& value) {
  fcl::Matrix3d rotation;
  for (int row = 0; row < 3; ++row)
    for (int column = 0; column < 3; ++column)
      rotation(row, column) = value.rotation[static_cast<std::size_t>(row * 3 + column)];
  fcl::Transform3d transform = fcl::Transform3d::Identity();
  transform.linear() = rotation;
  transform.translation() = fcl::Vector3d{value.translation_meters.x, value.translation_meters.y,
                                          value.translation_meters.z};
  return transform;
}
} // namespace
ValidationResult<CollisionModel> CollisionEngine::compile(TriangleMesh mesh) const {
  std::vector<ValidationError> errors;
  if (mesh.vertices_meters.empty())
    errors.push_back(
        error(ValidationCode::invalid_range, "vertices_meters", "vertices must not be empty"));
  if (mesh.triangles.empty())
    errors.push_back(
        error(ValidationCode::invalid_range, "triangles", "triangles must not be empty"));
  for (std::size_t i = 0; i < mesh.vertices_meters.size(); ++i) {
    const auto& v = mesh.vertices_meters[i];
    if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z))
      errors.push_back(error(ValidationCode::non_finite,
                             "vertices_meters[" + std::to_string(i) + "]",
                             "vertex coordinates must be finite"));
  }
  for (std::size_t i = 0; i < mesh.triangles.size(); ++i) {
    const auto t = mesh.triangles[i];
    const std::string field = "triangles[" + std::to_string(i) + "]";
    if (t.a >= mesh.vertices_meters.size() || t.b >= mesh.vertices_meters.size() ||
        t.c >= mesh.vertices_meters.size()) {
      errors.push_back(
          error(ValidationCode::invalid_range, field, "triangle index is out of range"));
      continue;
    }
    if (t.a == t.b || t.a == t.c || t.b == t.c) {
      errors.push_back(
          error(ValidationCode::invalid_range, field, "triangle indices must be distinct"));
      continue;
    }
    const auto a = mesh.vertices_meters[t.a], b = mesh.vertices_meters[t.b],
               c = mesh.vertices_meters[t.c];
    const double ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z, vx = c.x - a.x, vy = c.y - a.y,
                 vz = c.z - a.z;
    const double cx = uy * vz - uz * vy, cy = uz * vx - ux * vz, cz = ux * vy - uy * vx;
    if (!std::isfinite(cx) || !std::isfinite(cy) || !std::isfinite(cz) ||
        cx * cx + cy * cy + cz * cz <=
            std::numeric_limits<double>::epsilon() * std::numeric_limits<double>::epsilon())
      errors.push_back(
          error(ValidationCode::invalid_range, field, "triangle area is numerically degenerate"));
  }
  if (!errors.empty())
    return ValidationResult<CollisionModel>::failure(std::move(errors));
  std::vector<fcl::Vector3d> vertices;
  vertices.reserve(mesh.vertices_meters.size());
  for (const auto& v : mesh.vertices_meters)
    vertices.emplace_back(v.x, v.y, v.z);
  std::vector<fcl::Triangle> triangles;
  triangles.reserve(mesh.triangles.size());
  for (const auto& t : mesh.triangles)
    triangles.emplace_back(t.a, t.b, t.c);
  auto bvh = std::make_shared<Bvh>();
  if (bvh->beginModel(static_cast<int>(triangles.size()), static_cast<int>(vertices.size())) !=
          fcl::BVH_OK ||
      bvh->addSubModel(vertices, triangles) != fcl::BVH_OK || bvh->endModel() != fcl::BVH_OK)
    return ValidationResult<CollisionModel>::failure(
        {error(ValidationCode::invalid_range, "mesh", "FCL rejected the validated mesh")});
  auto implementation = std::make_shared<const CollisionModel::Implementation>(
      CollisionModel::Implementation{std::move(bvh), vertices.size(), triangles.size()});
  return ValidationResult<CollisionModel>::success(CollisionModel{std::move(implementation)});
}
ValidationResult<SinglePoseCollisionResult>
CollisionEngine::query(const CollisionModel& fixed, const CollisionModel& moving,
                       const occlusion::core::RigidTransform& moving_transform) const {
  const auto valid = occlusion::core::validate(moving_transform);
  if (!valid)
    return ValidationResult<SinglePoseCollisionResult>::failure(valid.errors());
  auto fixed_transform = fcl::Transform3d::Identity();
  auto moving_fcl_transform = transform_of(moving_transform);
  fcl::CollisionObjectd fixed_object(fixed.implementation_->geometry, fixed_transform);
  fcl::CollisionObjectd moving_object(moving.implementation_->geometry, moving_fcl_transform);
  fcl::DistanceRequestd distance_request;
  distance_request.enable_signed_distance = true;
  fcl::DistanceResultd distance_result;
  const double distance =
      fcl::distance(&fixed_object, &moving_object, distance_request, distance_result);
  if (!std::isfinite(distance))
    return ValidationResult<SinglePoseCollisionResult>::failure(
        {error(ValidationCode::non_finite, "distance", "FCL returned a non-finite distance")});
  if (distance > contact_tolerance.value())
    return ValidationResult<SinglePoseCollisionResult>::success(
        {ContactClassification::separated, Meters{distance}, Meters{0}, false, {}});
  fcl::CollisionRequestd collision_request;
  collision_request.enable_contact = true;
  collision_request.num_max_contacts = 1024;
  fcl::CollisionResultd collision_result;
  fcl::collide(&fixed_object, &moving_object, collision_request, collision_result);
  std::vector<fcl::Contactd> contacts;
  contacts.reserve(std::min<std::size_t>(collision_result.numContacts(), 1024));
  collision_result.getContacts(contacts);
  double maximum = 0;
  std::vector<ContactCandidate> candidates;
  candidates.reserve(contacts.size());
  for (const auto& contact : contacts) {
    const auto finite_vector = [](const fcl::Vector3d& v) {
      return std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2]);
    };
    if (!std::isfinite(contact.penetration_depth) || !finite_vector(contact.pos) ||
        !finite_vector(contact.normal))
      return ValidationResult<SinglePoseCollisionResult>::failure(
          {error(ValidationCode::non_finite, "contacts", "FCL returned a non-finite contact")});
    const double norm = contact.normal.norm();
    if (!std::isfinite(norm) || norm <= std::numeric_limits<double>::epsilon())
      return ValidationResult<SinglePoseCollisionResult>::failure({error(
          ValidationCode::invalid_range, "contacts.normal", "FCL returned an invalid normal")});
    maximum = std::max(maximum, contact.penetration_depth);
    const auto n = contact.normal / norm;
    const auto classification = contact.penetration_depth > contact_tolerance.value()
                                    ? ContactClassification::penetrating
                                    : ContactClassification::touching;
    candidates.push_back({{contact.pos[0], contact.pos[1], contact.pos[2]},
                          {n[0], n[1], n[2]},
                          Meters{-contact.penetration_depth},
                          Meters{std::max(0.0, contact.penetration_depth)},
                          classification});
  }
  if (maximum > contact_tolerance.value())
    return ValidationResult<SinglePoseCollisionResult>::success({ContactClassification::penetrating,
                                                                 Meters{0}, Meters{maximum}, true,
                                                                 std::move(candidates)});
  return ValidationResult<SinglePoseCollisionResult>::success(
      {ContactClassification::touching, Meters{0}, Meters{0}, collision_result.isCollision(),
       std::move(candidates)});
}
} // namespace occlusion::collision
