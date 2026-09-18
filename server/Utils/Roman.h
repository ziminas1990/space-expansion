#pragma once

#include <string>
#include <type_traits>
#include <utility>

namespace utils {

template <typename T>
constexpr bool is_roman_integer_v =
    std::is_integral_v<T> &&
    std::is_unsigned_v<T> &&
    !std::is_same_v<std::remove_cv_t<T>, bool> &&
    sizeof(T) <= 2;

template <typename T, std::enable_if_t<is_roman_integer_v<T>, int> = 0>
std::string toRoman(T value)
{
    static constexpr std::pair<unsigned, const char*> table[] = {
        {1000, "M"}, {900, "CM"},
        {500,  "D"}, {400, "CD"},
        {100,  "C"}, {90,  "XC"},
        {50,   "L"}, {40,  "XL"},
        {10,   "X"}, {9,   "IX"},
        {5,    "V"}, {4,   "IV"},
        {1,    "I"},
    };

    std::string result;

    for (auto [number, roman] : table) {
        while (value >= number) {
            result += roman;
            value -= number;
        }
    }

    return result;
}

} // namespace utils
