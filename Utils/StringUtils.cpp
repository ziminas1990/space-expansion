#include "StringUtils.h"

namespace utils {

void StringUtils::split(char separator, std::string const& source,
                        std::string &left, std::string &right)
{
  size_t nSeparatorIdx = source.find(separator);
  if (nSeparatorIdx == std::string::npos) {
    left = source;
  } else {
    left  = source.substr(0, nSeparatorIdx);
    right = source.substr(nSeparatorIdx + 1);
  }
}

} // namespace utils
