#pragma once

#include <vector>

namespace world {

struct Resources {
  static bool initialize();

  enum Type {
    eMettal,
    eSilicate,
    eIce,

    // total number of resources
    eTotalResources,
    eUnknown
  };

  static std::vector<double> density;
};

} // namespace world
