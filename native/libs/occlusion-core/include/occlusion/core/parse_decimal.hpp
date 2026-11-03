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

  std::istringstream stream{std::string{text}};
  stream.imbue(std::locale::classic());
  stream >> std::noskipws;
  double value = 0;
  if (!(stream >> value) || stream.peek() != std::char_traits<char>::eof())
    return std::nullopt;
  return value;
}

} // namespace occlusion::core
