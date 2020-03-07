#include "Resources.h"

#include <assert.h>
#include <map>
#include <utility>

#include <yaml-cpp/yaml.h>

#include <Utils/FloatComparator.h>

namespace world {

std::array<Resource::Type, Resource::eTotalResources> Resource::AllTypes = {
  Resource::eMetal, Resource::eSilicate, Resource::eIce, Resource::eLabor
};
std::array<std::string, Resource::eTotalResources> Resource::Names = {
  "metals", "silicates", "ice", "labor"
};

std::array<double, Resource::eTotalResources> Resource::density = {0};
const std::array<Resource::Type, 3> Resource::MaterialResources = {
  Resource::eMetal, Resource::eSilicate, Resource::eIce
};
const std::array<Resource::Type, 1> Resource::NonMaterialResources = {
  Resource::eLabor
};

bool Resource::initialize()
{
  density[eMetal]    = 4500;  // Ti
  density[eIce]      = 916;
  density[eSilicate] = 2330;  // Si
  density[eLabor]    = 0;     // It is non material resource

  // Run-time checks:
  assert(AllTypes.size() == eTotalResources);

  if (MaterialResources.size() + NonMaterialResources.size() != eTotalResources) {
    assert("All resources must be either material or non-material!" == nullptr);
    return false;
  }

  for (Type eResource : MaterialResources) {
    if (!isMaterial(eResource)) {
      assert("isMaterisl() must return 'true' for material resource!" == nullptr);
      return false;
    }
  }

  for (Type eResource : NonMaterialResources) {
    if (isMaterial(eResource)) {
      assert("isMaterisl() must return 'false' for non-material resource!" == nullptr);
      return false;
    }
  }

  return true;
}

Resource::Type Resource::typeFromString(std::string const& sType)
{
  const static std::map<std::string, Type> table = {
    std::make_pair(Names[eMetal],    eMetal),
    std::make_pair(Names[eIce],      eIce),
    std::make_pair(Names[eSilicate], eSilicate),
    std::make_pair(Names[eLabor],    eLabor)
  };

  auto I = table.find(sType);
  assert(I != table.end());
  return I != table.end() ? I->second : eUnknown;
}

std::string const& Resource::typeToString(Resource::Type eType)
{
  const static std::string unknown("unknown");
  assert(eType < Resource::eTotalResources);
  return Names[eType];
}

bool ResourceItem::operator==(ResourceItem const& other) const
{
  return m_eType == other.m_eType
      && utils::AlmostEqual(m_nAmount, other.m_nAmount);
}

bool ResourceItem::load(std::pair<YAML::Node, YAML::Node> const& data)
{
  m_eType   = Resource::typeFromString(data.first.as<std::string>());
  m_nAmount = data.second.as<double>();

  assert(m_eType != Resource::eUnknown);
  assert(m_nAmount >= 0);
  return isValid() && m_nAmount >= 0;
}

void ResourceItem::dump(YAML::Node& out) const
{
  out[Resource::typeToString(m_eType)] = m_nAmount;
}

bool ResourcesArray::load(YAML::Node const& node)
{
  std::array<double, Resource::eTotalResources>& out = *this;

  assert(node.IsMap());
  for (Resource::Type eResource : Resource::AllTypes) {
    YAML::Node const& item = node[Resource::Names[eResource]];
    if (item.IsDefined() && item.IsScalar())
      out[eResource] = item.as<double>();
  }
  return true;
}

double ResourcesArray::calculateTotalVolume() const
{
  double volume = 0;
  for (Resource::Type eResource : Resource::MaterialResources) {
    volume += at(eResource) /  world::Resource::density[eResource];
  }
  return volume;
}

} // namespace world
