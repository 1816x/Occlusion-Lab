#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <locale>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace occlusion::core {

inline std::optional<double> parse_decimal(std::string_view text) {
  if (text.empty() || text.front() == '+')
    return std::nullopt;

  std::string lower{text};
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : static_cast<char>(c);
  });
  if (lower == "nan" || lower == "-nan")
    return std::numeric_limits<double>::quiet_NaN();
  if (lower == "inf" || lower == "infinity")
    return std::numeric_limits<double>::infinity();
  if (lower == "-inf" || lower == "-infinity")
    return -std::numeric_limits<double>::infinity();

  // libc++ streams also accept hexadecimal floating-point text. This boundary
  // accepts decimal coordinates only, independent of the platform's num_get.
  if (text.find_first_not_of("0123456789.eE+-") != std::string_view::npos)
    return std::nullopt;

  std::istringstream stream{std::string{text}};
  stream.imbue(std::locale::classic());
  stream >> std::noskipws;
  double value = 0;
  stream >> value;
  if (stream.fail()) {
    // libc++ marks representable subnormals as ERANGE but retains their value.
    // Accept only a fully consumed, finite, nonzero subnormal in this case.
    if (!stream.eof() || !std::isfinite(value) || value == 0 ||
        std::abs(value) >= std::numeric_limits<double>::min())
      return std::nullopt;
    stream.clear();
  }
  if (stream.peek() != std::char_traits<char>::eof())
    return std::nullopt;
  // Some standard-library streams silently round underflow to zero.
  // Preserve genuine zero mantissas, but reject lost nonzero coordinates.
  const auto mantissa = text.substr(0, text.find_first_of("eE"));
  if (value == 0 && mantissa.find_first_of("123456789") != std::string_view::npos)
    return std::nullopt;
  return value;
}

} // namespace occlusion::core
