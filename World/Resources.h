#pragma once

#include <vector>

namespace world {

struct Resource {
  static bool initialize();

  enum Type {
    eMettal,
    eSilicate,
    eIce,

    // total number of resources
    eTotalResources
  };

  static std::vector<double> density;
};

} // namespace world
