#pragma once
namespace occlusion::core {
class Meters final {
public:
  explicit constexpr Meters(double value) noexcept : value_(value) {}
  [[nodiscard]] constexpr double value() const noexcept { return value_; }

private:
  double value_;
};
class Millimeters final {
public:
  explicit constexpr Millimeters(double value) noexcept : value_(value) {}
  [[nodiscard]] constexpr double value() const noexcept { return value_; }

private:
  double value_;
};
[[nodiscard]] constexpr Meters to_meters(Millimeters value) noexcept {
  return Meters{value.value() / 1000.0};
}
[[nodiscard]] constexpr Millimeters to_millimeters(Meters value) noexcept {
  return Millimeters{value.value() * 1000.0};
}
} // namespace occlusion::core
