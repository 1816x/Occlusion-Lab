#include "mesh_inspection.hpp"

#include "obj_files.hpp"
#include "occlusion/geometry/geometry.hpp"

#include <cstddef>
#include <utility>

namespace occlusion::cli {
namespace {
constexpr std::string_view prefix =
    "{\"schema\":\"occlusion-mesh-inspection\",\"schemaVersion\":1,";

void append_json_string(std::string& output, std::string_view value) {
  constexpr char digits[] = "0123456789abcdef";
  output += '"';
  for (const char raw : value) {
    const auto character = static_cast<unsigned char>(raw);
    switch (character) {
    case '"':
      output += "\\\"";
      break;
    case '\\':
      output += "\\\\";
      break;
    case '\b':
      output += "\\b";
      break;
    case '\f':
      output += "\\f";
      break;
    case '\n':
      output += "\\n";
      break;
    case '\r':
      output += "\\r";
      break;
    case '\t':
      output += "\\t";
      break;
    default:
      if (character < 0x20) {
        output += "\\u00";
        output += digits[character >> 4];
        output += digits[character & 0x0f];
      } else {
        output += raw;
      }
    }
  }
  output += '"';
}

InspectionResponse invalid_response(std::string_view code, std::string_view field, std::size_t line,
                                    std::string_view message) {
  std::string json{prefix};
  json += "\"status\":\"invalid\",\"error\":{\"code\":";
  append_json_string(json, code);
  json += ",\"field\":";
  append_json_string(json, field);
  json += ",\"line\":" + std::to_string(line) + ",\"message\":";
  append_json_string(json, message);
  json += "}}\n";
  return {false, std::move(json)};
}

std::string_view geometry_code(geometry::GeometryErrorCode code) {
  switch (code) {
  case geometry::GeometryErrorCode::invalid_mesh:
    return "invalid_mesh";
  case geometry::GeometryErrorCode::conversion_failed:
    return "conversion_failed";
  case geometry::GeometryErrorCode::degenerate_face:
    return "degenerate_face";
  case geometry::GeometryErrorCode::self_intersection:
    return "self_intersection";
  }
  return "unknown_error";
}
} // namespace

InspectionResponse inspect_synthetic_obj(const std::filesystem::path& directory,
                                         std::string_view name) {
  ObjFileError input_error;
  const auto mesh = load_synthetic_obj(directory, name, input_error);
  if (!mesh)
    return invalid_response(input_error.code, input_error.field, input_error.line,
                            input_error.message);
  if (const auto problem = geometry::inspect_mesh(*mesh))
    return invalid_response(geometry_code(problem->code), problem->field, 0, problem->message);
  return {true, std::string{prefix} + "\"status\":\"valid\"}\n"};
}
} // namespace occlusion::cli
