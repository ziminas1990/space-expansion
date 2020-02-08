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
    eTotalResources,
    eUnknown
  };

  static std::vector<double> density;
};

struct ResourceItem {
  ResourceItem(Resource::Type eType, double nAmount)
    : m_eType(eType), m_nAmount(nAmount)
  {}

  Resource::Type m_eType;
  double         m_nAmount;
};

using Resources = std::vector<ResourceItem>;

} // namespace world
