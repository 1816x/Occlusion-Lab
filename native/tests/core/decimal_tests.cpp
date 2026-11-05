#include "occlusion/core/parse_decimal.hpp"
#include <cmath>
#include <gtest/gtest.h>

using occlusion::core::parse_decimal;

TEST(DecimalParsing, RejectsOverflowAndNonzeroUnderflow) {
  for (const auto text : {"1e999", "-1e999", "1e-999", "-1e-999", "0.0001e-999"})
    EXPECT_FALSE(parse_decimal(text)) << text;
}

TEST(DecimalParsing, PreservesZeroAndRepresentableSubnormalValues) {
  for (const auto text : {"0", "0e-999", "-0.0e-999"}) {
    const auto value = parse_decimal(text);
    ASSERT_TRUE(value) << text;
    EXPECT_EQ(*value, 0);
  }
  const auto tiny = parse_decimal("1e-310");
  ASSERT_TRUE(tiny);
  EXPECT_GT(*tiny, 0);
  EXPECT_LT(*tiny, 1e-300);
}

TEST(DecimalParsing, RequiresCompleteLocaleIndependentToken) {
  EXPECT_EQ(parse_decimal("1.25e2"), 125);
  for (const auto text : {"", "+1", " 1", "1 ", "1,25", "1x", "0x1p2", "1e", "1e-310x"})
    EXPECT_FALSE(parse_decimal(text)) << text;
  ASSERT_TRUE(parse_decimal("NaN"));
  EXPECT_TRUE(std::isnan(*parse_decimal("NaN")));
  ASSERT_TRUE(parse_decimal("-INF"));
  EXPECT_TRUE(std::isinf(*parse_decimal("-INF")));
}
