#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>

#include "occlusion/core/domain.hpp"
#include "occlusion/core/length.hpp"

namespace {

constexpr std::string_view version = "Occlusion Lab native 0.4.0";

int self_check() {
  using namespace occlusion::core;
  constexpr double tolerance = 1e-12;
  const auto meters = to_meters(Millimeters{25.0});
  const auto millimeters = to_millimeters(meters);
  const MandibularPose valid_pose{meters, Meters{0.0}, Meters{0.0}};
  const MandibularPose invalid_pose{Meters{std::numeric_limits<double>::quiet_NaN()}, Meters{0.0},
                                    Meters{0.0}};

  const bool conversions_are_valid = std::abs(meters.value() - 0.025) <= tolerance &&
                                     std::abs(millimeters.value() - 25.0) <= tolerance;
  if (!conversions_are_valid || !validate(valid_pose) || validate(invalid_pose)) {
    std::cerr << "occlusion-cli self-check: FAIL\n";
    return 1;
  }

  std::cout << "occlusion-cli self-check: PASS\n";
  return 0;
}

} // namespace

int main(int argc, char* argv[]) {
  if (argc == 2 && std::string_view{argv[1]} == "--version") {
    std::cout << version << '\n';
    return 0;
  }
  if (argc == 2 && std::string_view{argv[1]} == "--self-check") {
    return self_check();
  }
  std::cerr << "Usage: occlusion-cli (--version | --self-check)\n";
  return 2;
}
