#pragma once

#include "occlusion/core/domain.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace occlusion::mesh_io {
inline constexpr std::size_t default_maximum_input_bytes = 16 * 1024 * 1024;
inline constexpr std::size_t default_maximum_vertices = 1'000'000;
inline constexpr std::size_t default_maximum_triangles = 2'000'000;

struct MeshIngestionLimits final {
  std::size_t maximum_input_bytes = default_maximum_input_bytes;
  std::size_t maximum_vertices = default_maximum_vertices;
  std::size_t maximum_triangles = default_maximum_triangles;
};

struct TriangleMesh final {
  std::vector<occlusion::core::Vector3> vertices_meters;
  std::vector<std::array<std::size_t, 3>> triangles;
};

enum class MeshIngestionErrorCode {
  empty_input,
  resource_limit,
  unsupported_statement,
  invalid_number,
  non_finite_coordinate,
  invalid_face,
  index_out_of_range,
  empty_mesh
};

struct MeshIngestionError final {
  MeshIngestionErrorCode code;
  std::size_t line;
  std::string field;
  std::string message;
  [[nodiscard]] bool operator==(const MeshIngestionError&) const = default;
};

template <typename T> class MeshIngestionResult final {
public:
  [[nodiscard]] static MeshIngestionResult success(T value) {
    return MeshIngestionResult{std::move(value), {}};
  }
  [[nodiscard]] static MeshIngestionResult failure(MeshIngestionError error) {
    return MeshIngestionResult{std::nullopt, std::move(error)};
  }
  [[nodiscard]] explicit operator bool() const noexcept { return value_.has_value(); }
  [[nodiscard]] const T& value() const { return value_.value(); }
  [[nodiscard]] const MeshIngestionError& error() const { return error_.value(); }

private:
  MeshIngestionResult(std::optional<T> value, std::optional<MeshIngestionError> error)
      : value_(std::move(value)), error_(std::move(error)) {}
  std::optional<T> value_;
  std::optional<MeshIngestionError> error_;
};

// Strict, dependency-free OBJ subset for synthetic triangle meshes in meters.
// Accepted records are comments, blank lines, `v x y z`, and positive one-based
// `f i j k` indices. Texture/normal indices, polygons, and relative indices fail.
[[nodiscard]] MeshIngestionResult<TriangleMesh> parse_obj_mesh(std::string_view source,
                                                               MeshIngestionLimits limits = {});
} // namespace occlusion::mesh_io
