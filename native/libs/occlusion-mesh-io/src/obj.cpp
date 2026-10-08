#include "occlusion/mesh_io/mesh_io.hpp"

#include <charconv>
#include <cmath>

namespace occlusion::mesh_io {
namespace {
std::string_view trim(std::string_view value) {
  const auto first = value.find_first_not_of(" \t\r");
  if (first == std::string_view::npos)
    return {};
  const auto last = value.find_last_not_of(" \t\r");
  return value.substr(first, last - first + 1);
}

std::optional<std::string_view> take_token(std::string_view& value) {
  value = trim(value);
  if (value.empty() || value.front() == '#')
    return std::nullopt;
  const auto end = value.find_first_of(" \t\r#");
  const auto token = value.substr(0, end);
  value = end == std::string_view::npos ? std::string_view{} : value.substr(end);
  return token;
}

template <typename T> std::optional<T> parse_number(std::string_view token) {
  T value{};
  const auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), value);
  if (error != std::errc{} || end != token.data() + token.size())
    return std::nullopt;
  return value;
}

MeshIngestionResult<TriangleMesh> failure(MeshIngestionErrorCode code, std::size_t line,
                                          std::string field, std::string message) {
  return MeshIngestionResult<TriangleMesh>::failure(
      {code, line, std::move(field), std::move(message)});
}
} // namespace

MeshIngestionResult<TriangleMesh> parse_obj_mesh(std::string_view source,
                                                 MeshIngestionLimits limits) {
  if (source.empty())
    return failure(MeshIngestionErrorCode::empty_input, 0, "source", "must not be empty");
  if (source.size() > limits.maximum_input_bytes)
    return failure(MeshIngestionErrorCode::resource_limit, 0, "source",
                   "exceeds maximum input bytes");

  TriangleMesh mesh;
  std::size_t line_number = 0;
  while (!source.empty()) {
    ++line_number;
    const auto newline = source.find('\n');
    auto line = trim(source.substr(0, newline));
    source = newline == std::string_view::npos ? std::string_view{} : source.substr(newline + 1);
    if (line.empty() || line.front() == '#')
      continue;

    const auto statement = take_token(line);
    if (!statement)
      continue;
    if (*statement == "v") {
      std::array<double, 3> coordinates{};
      for (std::size_t index = 0; index < coordinates.size(); ++index) {
        const auto token = take_token(line);
        if (!token)
          return failure(MeshIngestionErrorCode::invalid_number, line_number, "vertex",
                         "requires exactly three coordinates");
        const auto coordinate = parse_number<double>(*token);
        if (!coordinate)
          return failure(MeshIngestionErrorCode::invalid_number, line_number, "vertex",
                         "coordinate is not a decimal number");
        if (!std::isfinite(*coordinate))
          return failure(MeshIngestionErrorCode::non_finite_coordinate, line_number, "vertex",
                         "coordinate must be finite");
        coordinates[index] = *coordinate;
      }
      if (take_token(line))
        return failure(MeshIngestionErrorCode::invalid_number, line_number, "vertex",
                       "requires exactly three coordinates");
      if (mesh.vertices_meters.size() >= limits.maximum_vertices)
        return failure(MeshIngestionErrorCode::resource_limit, line_number, "vertices",
                       "exceeds maximum vertex count");
      mesh.vertices_meters.push_back({coordinates[0], coordinates[1], coordinates[2]});
      continue;
    }
    if (*statement == "f") {
      std::array<std::size_t, 3> triangle{};
      for (std::size_t index = 0; index < triangle.size(); ++index) {
        const auto token = take_token(line);
        if (!token || token->find('/') != std::string_view::npos)
          return failure(MeshIngestionErrorCode::invalid_face, line_number, "face",
                         "requires exactly three plain positive indices");
        const auto parsed = parse_number<std::size_t>(*token);
        if (!parsed || *parsed == 0)
          return failure(MeshIngestionErrorCode::invalid_face, line_number, "face",
                         "indices must be positive and one-based");
        if (*parsed > mesh.vertices_meters.size())
          return failure(MeshIngestionErrorCode::index_out_of_range, line_number, "face",
                         "index must reference a preceding vertex");
        triangle[index] = *parsed - 1;
      }
      if (take_token(line))
        return failure(MeshIngestionErrorCode::invalid_face, line_number, "face",
                       "polygons are not accepted; triangulate before ingestion");
      if (triangle[0] == triangle[1] || triangle[1] == triangle[2] || triangle[0] == triangle[2])
        return failure(MeshIngestionErrorCode::invalid_face, line_number, "face",
                       "triangle indices must be distinct");
      if (mesh.triangles.size() >= limits.maximum_triangles)
        return failure(MeshIngestionErrorCode::resource_limit, line_number, "triangles",
                       "exceeds maximum triangle count");
      mesh.triangles.push_back(triangle);
      continue;
    }
    return failure(MeshIngestionErrorCode::unsupported_statement, line_number, "statement",
                   "only v and f records are accepted");
  }
  if (mesh.vertices_meters.empty() || mesh.triangles.empty())
    return failure(MeshIngestionErrorCode::empty_mesh, line_number, "mesh",
                   "requires at least one vertex and triangle");
  return MeshIngestionResult<TriangleMesh>::success(std::move(mesh));
}
} // namespace occlusion::mesh_io
