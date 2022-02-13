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
    eStone,   // Useless resource

    // Non-material resources:
    eLabor, // some amount of work/producing

    // service fields
    eTotalResources,
    eUnknown
  };

  static bool isMaterial(Type eType) { return Density[eType] > 0.0; }

  static Type        typeFromString(std::string const& sType);
  static char const* typeToString(Type eType);

  static const std::array<Type, eTotalResources>        AllTypes;
  static const std::array<const char*, eTotalResources> Names;
  static       std::array<double, eTotalResources>      Density;

  static const std::array<Type, 4> MaterialResources;
  static const std::array<Type, 1> NonMaterialResources;
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

  bool load(std::pair<YAML::Node, YAML::Node> const& data);
  void dump(YAML::Node &out) const;

  Resource::Type m_eType;
  double         m_nAmount;
};

using Resources = std::vector<ResourceItem>;

// Stores resources in array, where index is resource type and value is amout.
// Note that this class can NOT be inherited, because it doesn't have virtual
// destructor!
class ResourcesArray : public std::array<double, Resource::eTotalResources>
{
public:
  ResourcesArray() { fill(0); }

  bool load(YAML::Node const& node);
  void dump(YAML::Node &out) const;

  ResourcesArray& metals(double amount);
  ResourcesArray& silicates(double amount);
  ResourcesArray& ice(double amount);
  ResourcesArray& stones(double amount);

  double metals() const { return at(Resource::eMetal); }
  double silicates() const { return at(Resource::eSilicate); }
  double ice() const { return at(Resource::eIce); }
  double stones() const { return at(Resource::eStone); }

  ResourcesArray& operator+=(ResourcesArray const& other);
  bool operator==(ResourcesArray const& other) const;

  double calculateTotalMass() const;
  double calculateTotalVolume() const;

  void normalize();
    // Each resource takes value from 0 to 1, depending on it's initial amount.
    // Total amount of all resources turns to 1.

  ResourcesArray normalized() const;
};

} // namespace world
