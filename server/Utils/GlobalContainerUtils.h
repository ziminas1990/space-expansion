#pragma once

#include <iosfwd>

namespace utils {

class GlobalContainerUtils
{
public:
  static bool checkAllContainersAreEmpty(std::ostream& problem);
};

} // namespace utils