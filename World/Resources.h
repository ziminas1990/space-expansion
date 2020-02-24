#pragma once

#include <vector>
#include <string>
#include <array>
#include <Utils/YamlForwardDeclarations.h>

namespace world {

struct Resource {
  static bool initialize();

  enum Type {
    // Material resources:
    eMetal,
    eSilicate,
    eIce,

    // Non-material resources:
    eLabor, // some amount of work/producing

    // service fields
    eTotalResources,
    eUnknown
  };

  static bool isMaterial(Type eType) { return density[eType] > 0.0; }

  static Type               typeFromString(std::string const& sType);
  static std::string const& typeToString(Type eType);

  static std::array<double, eTotalResources> density;
  static const std::array<Type, 3> MaterialResources;
  static const std::array<Type, 1> NonMaterialResources;
    // Only material resources, that can be put to container
};

struct ResourceItem {
  static ResourceItem metals(double nAmount) {
    return ResourceItem(Resource::eMetal, nAmount);
  }

  static ResourceItem silicates(double nAmount) {
    return ResourceItem(Resource::eSilicate, nAmount);
  }

  static ResourceItem ice(double nAmount) {
    return ResourceItem(Resource::eIce, nAmount);
  }

  static ResourceItem labor(double nAmount) {
    return ResourceItem(Resource::eLabor, nAmount);
  }

  ResourceItem()
    : m_eType(Resource::eUnknown), m_nAmount(0)
  {}

  ResourceItem(Resource::Type eType, double nAmount)
    : m_eType(eType), m_nAmount(nAmount)
  {}

  bool operator==(ResourceItem const& other) const;

  bool isValid() const { return m_eType != Resource::eUnknown; }

  bool load(const std::pair<YAML::Node, YAML::Node> &data);
  void dump(YAML::Node &out) const;

  Resource::Type m_eType;
  double         m_nAmount;
};

using Resources      = std::vector<ResourceItem>;

using ResourcesArray = std::array<double, Resource::eTotalResources>;
  // Resource type is index, amount is value

} // namespace world
