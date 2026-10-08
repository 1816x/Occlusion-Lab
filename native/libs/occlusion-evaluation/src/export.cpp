#include "occlusion/evaluation/evaluation.hpp"

#include <array>
#include <charconv>
#include <iomanip>
#include <locale>
#include <sstream>

namespace occlusion::evaluation {
namespace {
using occlusion::core::ContactClassification;
using occlusion::core::MandibularPose;
using occlusion::core::RigidTransform;

std::string_view classification_name(ContactClassification value) {
  switch (value) {
  case ContactClassification::separated:
    return "separated";
  case ContactClassification::touching:
    return "touching";
  case ContactClassification::penetrating:
    return "penetrating";
  }
  return "unknown";
}

std::string_view preset_name(occlusion::core::SweepPreset value) {
  switch (value) {
  case occlusion::core::SweepPreset::closing:
    return "closing";
  case occlusion::core::SweepPreset::protrusive:
    return "protrusive";
  case occlusion::core::SweepPreset::left_lateral:
    return "left-lateral";
  case occlusion::core::SweepPreset::right_lateral:
    return "right-lateral";
  }
  return "unknown";
}

void write_string(std::ostream& output, std::string_view value) {
  output << '"';
  for (const char raw_character : value) {
    const auto character = static_cast<unsigned char>(raw_character);
    switch (character) {
    case '"':
      output << "\\\"";
      break;
    case '\\':
      output << "\\\\";
      break;
    case '\b':
      output << "\\b";
      break;
    case '\f':
      output << "\\f";
      break;
    case '\n':
      output << "\\n";
      break;
    case '\r':
      output << "\\r";
      break;
    case '\t':
      output << "\\t";
      break;
    default:
      if (character < 0x20)
        output << "\\u00" << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<unsigned int>(character) << std::dec << std::setfill(' ');
      else
        output << static_cast<char>(character);
    }
  }
  output << '"';
}

void write_number(std::ostream& output, double value) {
  std::array<char, 32> buffer{};
  const auto [end, error] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
  if (error == std::errc{})
    output.write(buffer.data(), end - buffer.data());
  else
    output << "null";
}

void write_pose(std::ostream& output, const MandibularPose& pose) {
  output << "{\"opening\":";
  write_number(output, pose.opening.value());
  output << ",\"protrusion\":";
  write_number(output, pose.protrusion.value());
  output << ",\"lateral\":";
  write_number(output, pose.lateral_displacement.value());
  output << '}';
}

void write_transform(std::ostream& output, const RigidTransform& transform) {
  output << "{\"translationMeters\":{\"x\":";
  write_number(output, transform.translation_meters.x);
  output << ",\"y\":";
  write_number(output, transform.translation_meters.y);
  output << ",\"z\":";
  write_number(output, transform.translation_meters.z);
  output << "},\"rotationMatrix\":[";
  for (std::size_t index = 0; index < transform.rotation.size(); ++index) {
    if (index != 0)
      output << ',';
    write_number(output, transform.rotation[index]);
  }
  output << "]}";
}

void write_optional_index(std::ostream& output, std::optional<std::size_t> value) {
  if (value)
    output << *value;
  else
    output << "null";
}

void write_evaluation(std::ostream& output, const PoseEvaluationResult& result) {
  output << "\"requestedPoseMeters\":";
  write_pose(output, result.requested_pose);
  output << ",\"appliedTransform\":";
  write_transform(output, result.applied_transform);
  output << ",\"classification\":\"" << classification_name(result.classification)
         << "\",\"measurementStatus\":\""
         << (result.measurement_status == MeasurementStatus::available ? "available"
                                                                       : "unavailable")
         << "\",\"clearanceMeters\":";
  if (result.clearance)
    write_number(output, result.clearance->value());
  else
    output << "null";
  output << ",\"penetrationDepthMeters\":";
  write_number(output, result.penetration_depth.value());
  output << ",\"intersects\":";
  if (result.intersects)
    output << (*result.intersects ? "true" : "false");
  else
    output << "null";
  output << ",\"contactCount\":" << result.normalized_contact_count << ",\"contacts\":[";
  for (std::size_t index = 0; index < result.contacts.size(); ++index) {
    if (index != 0)
      output << ',';
    const auto& contact = result.contacts[index];
    output << "{\"id\":";
    write_string(output, contact.stable_id);
    output << ",\"positionMeters\":{\"x\":";
    write_number(output, contact.position_meters.x);
    output << ",\"y\":";
    write_number(output, contact.position_meters.y);
    output << ",\"z\":";
    write_number(output, contact.position_meters.z);
    output << "},\"normalFixedToMoving\":{\"x\":";
    write_number(output, contact.normal_fixed_to_moving.x);
    output << ",\"y\":";
    write_number(output, contact.normal_fixed_to_moving.y);
    output << ",\"z\":";
    write_number(output, contact.normal_fixed_to_moving.z);
    output << "},\"penetrationDepthMeters\":";
    write_number(output, contact.penetration_depth.value());
    output << ",\"classification\":\"" << classification_name(contact.classification) << "\"}";
  }
  output << ']';
}

std::ostringstream document_start(std::string_view fixture_id, std::string_view kind) {
  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << "{\"schema\":\"occlusion-native-evaluation\",\"schemaVersion\":"
         << evaluation_export_schema_version << ",\"fixtureId\":";
  write_string(output, fixture_id);
  output << ",\"kind\":";
  write_string(output, kind);
  output << ",\"units\":{\"length\":\"meters\",\"rotation\":\"row-major-3x3\"},"
            "\"manifoldParityGuaranteed\":false,";
  return output;
}
} // namespace

std::string serialize_evaluation_json(const PoseEvaluationResult& result,
                                      std::string_view fixture_id) {
  auto output = document_start(fixture_id, "pose");
  write_evaluation(output, result);
  output << "}\n";
  return output.str();
}

std::string serialize_evaluation_json(const EvaluatedSweepResult& result,
                                      std::string_view fixture_id) {
  auto output = document_start(fixture_id, "sweep");
  output << "\"preset\":\"" << preset_name(result.preset)
         << "\",\"requestedFrameCount\":" << result.requested_frame_count
         << ",\"finalPoseMeters\":";
  write_pose(output, result.final_pose);
  output << ",\"summary\":{\"totalFrameCount\":" << result.summary.total_frame_count
         << ",\"firstContactFrame\":";
  write_optional_index(output, result.summary.first_contact_frame);
  output << ",\"lastContactFrame\":";
  write_optional_index(output, result.summary.last_contact_frame);
  output << ",\"contactFrameCount\":" << result.summary.contact_frame_count
         << ",\"maximumPenetrationMeters\":";
  write_number(output, result.summary.maximum_penetration.value());
  output << ",\"maximumPenetrationFrame\":" << result.summary.maximum_penetration_frame
         << ",\"contactPersistsThroughFinalFrame\":"
         << (result.summary.contact_persists_through_final_frame ? "true" : "false")
         << "},\"frames\":[";
  for (std::size_t index = 0; index < result.frames.size(); ++index) {
    if (index != 0)
      output << ',';
    const auto& frame = result.frames[index];
    output << "{\"frameIndex\":" << frame.index << ",\"progress\":";
    write_number(output, frame.normalized_progress);
    output << ',';
    write_evaluation(output, frame.evaluation);
    output << '}';
  }
  output << "]}\n";
  return output.str();
}
} // namespace occlusion::evaluation
