#pragma once

#include <string>
#include <sstream>

namespace utils {

class StringUtils {
public:
    static void split(char separator,
        std::string_view source,
        std::string& left,
        std::string& right);

    static bool startsWith(std::string const& sLongString,
        std::string const& sExpectedPrefix);

    template<typename T>
    static std::string concat(const T& v)
    {
        std::stringstream ss;
        ss << v;
        return ss.str();
    }

    template<typename T, typename... Targs>
    static std::string concat(const T& v, Targs... args)
    {
        std::stringstream ss;
        ss << v << concat(args...);
        return ss.str();
    }

    template<typename T>
    static std::string join(const T& array, std::string_view separator)
    {
        std::stringstream ss;
        bool first = true;
        for (const auto& item : array) {
            if (!first) {
                ss << separator;
            }
            first = false;
            ss << item;
        }
        return ss.str();
    }

};

} // namespace utils
