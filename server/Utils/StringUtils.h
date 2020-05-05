#pragma once

#include <string>

namespace utils {

class StringUtils {
public:
  static void split(char separator, std::string const& source,
                    std::string& left, std::string& right);

  static bool startsWith(std::string const& sLongString,
                         std::string const& sExpectedPrefix);

};

} // namespace utils
