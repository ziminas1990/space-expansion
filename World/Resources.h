#pragma once

#include <vector>
#include <string>
#include <Utils/YamlForwardDeclarations.h>

namespace world {

struct Resource {
  static bool initialize();

  enum Type {
    eMettal,
    eSilicate,
    eIce,
    eLabor, // some amount of work/producing

    // total number of resources
    eTotalResources,
    eUnknown
  };

  static Type               typeFromString(std::string const& sType);
  static std::string const& typeToString(Type eType);

  static std::vector<double> density;
};

struct ResourceItem {
  ResourceItem()
    : m_eType(Resource::eUnknown), m_nAmount(0)
  {}

  ResourceItem(Resource::Type eType, double nAmount)
    : m_eType(eType), m_nAmount(nAmount)
  {}

  bool isValid() const { return m_eType != Resource::eUnknown; }

  bool load(const std::pair<YAML::Node, YAML::Node> &data);
  void dump(YAML::Node &out) const;

  Resource::Type m_eType;
  double         m_nAmount;
};

using Resources = std::vector<ResourceItem>;

} // namespace world
