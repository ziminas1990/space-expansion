#include "StringUtils.h"

#include <cstring>

namespace utils {

void StringUtils::split(char separator, std::string const& source,
                        std::string &left, std::string &right)
{
  size_t nSeparatorIdx = source.find(separator);
  if (nSeparatorIdx == std::string::npos) {
    left = source;
    right.clear();
  } else {
    left  = source.substr(0, nSeparatorIdx);
    right = source.substr(nSeparatorIdx + 1);
  }
}

bool StringUtils::startsWith(std::string const& sLongString,
                             std::string const& sExpectedPrefix)
{
  if (sExpectedPrefix.empty())
    return true;  // every string starts with empty string

  const size_t nPrefixLength = sExpectedPrefix.size();
  if (sLongString.size() < nPrefixLength)
    return false;

  return 0 == std::memcmp(sLongString.data(),
                          sExpectedPrefix.data(),
                          sExpectedPrefix.length());
}

} // namespace utils
