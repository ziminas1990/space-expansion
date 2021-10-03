#pragma once

#include <memory>
#include <vector>

namespace utils
{

template<typename T>
inline void removeExpiredWeakPtrs(std::vector<std::weak_ptr<T>>& from)
{
  for(size_t i = 0; i < from.size(); ++i) {
    if (from[i].expired()) {
      // To remove element, just swap it with the last element and than remove last
      // element
      if (i < from.size() - 1)
        std::swap(from.at(i), from.back());
      from.pop_back();
    }
  }
}

} // namespace utils
