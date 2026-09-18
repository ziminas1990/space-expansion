#include <gtest/gtest.h>

#include <cstdint>
#include <string>

#include <Utils/Roman.h>

TEST(RomanTests, ConvertsKnownValues)
{
  struct Case {
    uint16_t    value;
    const char* roman;
  };

  const Case cases[] = {
      {1,    "I"},
      {2,    "II"},
      {3,    "III"},
      {4,    "IV"},
      {5,    "V"},
      {9,    "IX"},
      {10,   "X"},
      {40,   "XL"},
      {44,   "XLIV"},
      {49,   "XLIX"},
      {50,   "L"},
      {90,   "XC"},
      {99,   "XCIX"},
      {100,  "C"},
      {400,  "CD"},
      {444,  "CDXLIV"},
      {500,  "D"},
      {900,  "CM"},
      {999,  "CMXCIX"},
      {1000, "M"},
  };

  // 1. convert each value to roman numerals
  for (const Case& row : cases) {
    EXPECT_EQ(std::string(row.roman), utils::toRoman(row.value))
        << row.value;
  }
}
