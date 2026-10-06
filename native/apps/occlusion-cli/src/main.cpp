#include <charconv>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include "occlusion/core/domain.hpp"
#include "occlusion/core/length.hpp"
#ifdef OCCLUSION_CLI_ENABLE_EVALUATION
#include "occlusion/evaluation/evaluation.hpp"
#endif

namespace {

constexpr std::string_view version = "Occlusion Lab native 0.4.0";
constexpr std::string_view usage =
    "Usage: occlusion-cli (--version | --self-check | --evaluate-fixture pose "
    "<opening-mm> <protrusion-mm> <lateral-mm> | --evaluate-fixture sweep "
    "<closing|protrusive|left-lateral|right-lateral> <frames>)";

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

#ifdef OCCLUSION_CLI_ENABLE_EVALUATION
using occlusion::collision::TriangleMesh;
using occlusion::core::MandibularPose;
using occlusion::core::Millimeters;
using occlusion::core::SweepPreset;
using occlusion::evaluation::PoseEvaluator;

TriangleMesh synthetic_box() {
  constexpr double half_extent = 0.05;
  return {{{-half_extent, -half_extent, -half_extent},
           {half_extent, -half_extent, -half_extent},
           {half_extent, half_extent, -half_extent},
           {-half_extent, half_extent, -half_extent},
           {-half_extent, -half_extent, half_extent},
           {half_extent, -half_extent, half_extent},
           {half_extent, half_extent, half_extent},
           {-half_extent, half_extent, half_extent}},
          {{0, 2, 1},
           {0, 3, 2},
           {4, 5, 6},
           {4, 6, 7},
           {0, 1, 5},
           {0, 5, 4},
           {3, 7, 6},
           {3, 6, 2},
           {0, 4, 7},
           {0, 7, 3},
           {1, 2, 6},
           {1, 6, 5}}};
}

template <typename Value> std::optional<Value> parse_number(std::string_view text) {
  Value value{};
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (error != std::errc{} || end != text.data() + text.size())
    return std::nullopt;
  if constexpr (std::is_floating_point_v<Value>) {
    if (!std::isfinite(value))
      return std::nullopt;
  }
  return value;
}

std::optional<SweepPreset> parse_preset(std::string_view value) {
  if (value == "closing")
    return SweepPreset::closing;
  if (value == "protrusive")
    return SweepPreset::protrusive;
  if (value == "left-lateral")
    return SweepPreset::left_lateral;
  if (value == "right-lateral")
    return SweepPreset::right_lateral;
  return std::nullopt;
}

std::optional<PoseEvaluator> make_synthetic_evaluator() {
  auto evaluator = PoseEvaluator::create(synthetic_box(), synthetic_box());
  if (!evaluator) {
    std::cerr << "Failed to compile the built-in synthetic fixture.\n";
    return std::nullopt;
  }
  return evaluator.value();
}

int evaluate_pose(int argc, char* argv[]) {
  if (argc != 6) {
    std::cerr << usage << '\n';
    return 2;
  }
  const auto opening = parse_number<double>(argv[3]);
  const auto protrusion = parse_number<double>(argv[4]);
  const auto lateral = parse_number<double>(argv[5]);
  if (!opening || !protrusion || !lateral) {
    std::cerr << "Pose values must be finite decimal millimeters.\n";
    return 2;
  }
  const MandibularPose pose{occlusion::core::to_meters(Millimeters{*opening}),
                            occlusion::core::to_meters(Millimeters{*protrusion}),
                            occlusion::core::to_meters(Millimeters{*lateral})};
  auto evaluator = make_synthetic_evaluator();
  if (!evaluator)
    return 1;
  const auto result = evaluator->evaluate(pose);
  if (!result) {
    std::cerr << "Pose is outside the validated native domain.\n";
    return 2;
  }
  std::cout << occlusion::evaluation::serialize_evaluation_json(result.value(),
                                                                "synthetic-boxes-v1");
  return 0;
}

int evaluate_sweep(int argc, char* argv[]) {
  if (argc != 5) {
    std::cerr << usage << '\n';
    return 2;
  }
  const auto preset = parse_preset(argv[3]);
  const auto frames = parse_number<std::size_t>(argv[4]);
  if (!preset || !frames) {
    std::cerr << "Sweep preset or frame count is invalid.\n";
    return 2;
  }
  auto evaluator = make_synthetic_evaluator();
  if (!evaluator)
    return 1;
  const auto result = evaluator->evaluate_sweep(*preset, *frames);
  if (!result) {
    std::cerr << "Frame count must be within the validated range [2, 61].\n";
    return 2;
  }
  std::cout << occlusion::evaluation::serialize_evaluation_json(result.value(),
                                                                "synthetic-boxes-v1");
  return 0;
}

int evaluate_fixture(int argc, char* argv[]) {
  if (argc >= 3 && std::string_view{argv[2]} == "pose")
    return evaluate_pose(argc, argv);
  if (argc >= 3 && std::string_view{argv[2]} == "sweep")
    return evaluate_sweep(argc, argv);
  std::cerr << usage << '\n';
  return 2;
}
#endif

} // namespace

int main(int argc, char* argv[]) {
  if (argc == 2 && std::string_view{argv[1]} == "--version") {
    std::cout << version << '\n';
    return 0;
  }
  if (argc == 2 && std::string_view{argv[1]} == "--self-check") {
    return self_check();
  }
#ifdef OCCLUSION_CLI_ENABLE_EVALUATION
  if (argc >= 2 && std::string_view{argv[1]} == "--evaluate-fixture")
    return evaluate_fixture(argc, argv);
#endif
  std::cerr << usage << '\n';
  return 2;
}
