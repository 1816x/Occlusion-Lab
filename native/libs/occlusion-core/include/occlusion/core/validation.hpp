#pragma once
#include <optional>
#include <string>
#include <utility>
#include <vector>
namespace occlusion::core {
enum class ValidationCode { non_finite, invalid_rotation, invalid_range, inconsistent_summary };
struct ValidationError final {
  ValidationCode code;
  std::string field;
  std::string message;
  [[nodiscard]] bool operator==(const ValidationError&) const = default;
};
template <typename T> class ValidationResult final {
public:
  [[nodiscard]] static ValidationResult success(T value) {
    return ValidationResult{std::move(value), {}};
  }
  [[nodiscard]] static ValidationResult failure(std::vector<ValidationError> errors) {
    return ValidationResult{std::nullopt, std::move(errors)};
  }
  [[nodiscard]] bool has_value() const noexcept { return value_.has_value(); }
  [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }
  [[nodiscard]] const T& value() const { return value_.value(); }
  [[nodiscard]] const std::vector<ValidationError>& errors() const noexcept { return errors_; }

private:
  ValidationResult(std::optional<T> value, std::vector<ValidationError> errors)
      : value_(std::move(value)), errors_(std::move(errors)) {}
  std::optional<T> value_;
  std::vector<ValidationError> errors_;
};
} // namespace occlusion::core
