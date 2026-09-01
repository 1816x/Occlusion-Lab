#include "occlusion/evaluation/evaluation.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <tuple>
#include <utility>

namespace occlusion::evaluation {
namespace {
using occlusion::core::ContactClassification;
using occlusion::core::ValidationCode;
using occlusion::core::ValidationError;
using occlusion::core::ValidationResult;
using occlusion::core::Vector3;
using Key = std::array<std::int64_t, 3>;
constexpr double scale = 1'000'000.0;
bool finite(Vector3 v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }
double quantize(double v) { return std::round(v * scale) / scale; }
Vector3 quantize(Vector3 v) { return {quantize(v.x), quantize(v.y), quantize(v.z)}; }
auto values(const NormalizedContactSample& s) {
  return std::tuple{s.position_meters.x,          s.position_meters.y,
                    s.position_meters.z,          s.normal_fixed_to_moving.x,
                    s.normal_fixed_to_moving.y,   s.normal_fixed_to_moving.z,
                    -s.penetration_depth.value(), static_cast<int>(s.classification)};
}
std::string id_for(const NormalizedContactSample& sample) {
  std::ostringstream canonical;
  canonical << std::fixed << std::setprecision(contact_quantization_decimal_places)
            << sample.position_meters.x << ',' << sample.position_meters.y << ','
            << sample.position_meters.z << ',' << sample.normal_fixed_to_moving.x << ','
            << sample.normal_fixed_to_moving.y << ',' << sample.normal_fixed_to_moving.z << ','
            << sample.penetration_depth.value() << ',' << static_cast<int>(sample.classification);
  std::uint64_t hash = 14695981039346656037ULL;
  for (const char character : canonical.str()) {
    const auto byte = static_cast<unsigned char>(character);
    hash ^= byte;
    hash *= 1099511628211ULL;
  }
  std::ostringstream id;
  id << "contact-" << std::hex << std::setfill('0') << std::setw(16) << hash;
  return id.str();
}
} // namespace
ValidationResult<std::vector<NormalizedContactSample>>
normalize_contacts(std::vector<occlusion::collision::ContactCandidate> candidates) {
  std::vector<ValidationError> errors;
  std::map<Key, NormalizedContactSample> unique;
  for (std::size_t i = 0; i < candidates.size(); ++i) {
    const auto& candidate = candidates[i];
    const std::string field = "candidates[" + std::to_string(i) + "]";
    if (!finite(candidate.position_meters) || !finite(candidate.normal_fixed_to_moving) ||
        !std::isfinite(candidate.signed_distance.value()) ||
        !std::isfinite(candidate.penetration_depth.value())) {
      errors.push_back({ValidationCode::non_finite, field, "all candidate values must be finite"});
      continue;
    }
    const auto n = candidate.normal_fixed_to_moving;
    const double magnitude = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
    if (!std::isfinite(magnitude) || magnitude <= std::numeric_limits<double>::epsilon()) {
      errors.push_back({ValidationCode::invalid_range, field + ".normal_fixed_to_moving",
                        "normal must have non-zero finite length"});
      continue;
    }
    if (candidate.penetration_depth.value() < 0.0) {
      errors.push_back({ValidationCode::invalid_range, field + ".penetration_depth",
                        "penetration depth must be non-negative"});
      continue;
    }
    NormalizedContactSample sample{
        "", quantize(candidate.position_meters),
        quantize({n.x / magnitude, n.y / magnitude, n.z / magnitude}),
        occlusion::core::Meters{quantize(candidate.penetration_depth.value())},
        candidate.classification};
    const Key key{static_cast<std::int64_t>(
                      std::floor(candidate.position_meters.x / contact_deduplication_grid_meters)),
                  static_cast<std::int64_t>(
                      std::floor(candidate.position_meters.y / contact_deduplication_grid_meters)),
                  static_cast<std::int64_t>(
                      std::floor(candidate.position_meters.z / contact_deduplication_grid_meters))};
    const auto found = unique.find(key);
    if (found == unique.end() ||
        sample.penetration_depth.value() > found->second.penetration_depth.value() ||
        (sample.penetration_depth.value() == found->second.penetration_depth.value() &&
         values(sample) < values(found->second)))
      unique.insert_or_assign(key, std::move(sample));
  }
  if (!errors.empty())
    return ValidationResult<std::vector<NormalizedContactSample>>::failure(std::move(errors));
  std::vector<NormalizedContactSample> normalized;
  normalized.reserve(std::min(unique.size(), maximum_contact_samples));
  for (auto& [key, sample] : unique) {
    (void)key;
    sample.stable_id = id_for(sample);
    normalized.push_back(std::move(sample));
  }
  std::sort(normalized.begin(), normalized.end(),
            [](const auto& a, const auto& b) { return values(a) < values(b); });
  if (normalized.size() > maximum_contact_samples)
    normalized.erase(normalized.begin() + static_cast<std::ptrdiff_t>(maximum_contact_samples),
                     normalized.end());
  return ValidationResult<std::vector<NormalizedContactSample>>::success(std::move(normalized));
}
} // namespace occlusion::evaluation
