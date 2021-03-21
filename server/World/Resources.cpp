#include "Resources.h"

#include <assert.h>
#include <map>
#include <utility>

#include <yaml-cpp/yaml.h>
#include <Utils/YamlDumper.h>

#include <Utils/FloatComparator.h>

namespace world {

const std::array<Resource::Type, Resource::eTotalResources> Resource::AllTypes = {
  Resource::eMetal,
  Resource::eSilicate,
  Resource::eIce,
  Resource::eStone,
  Resource::eLabor
};
const std::array<const char*, Resource::eTotalResources> Resource::Names = {
  "metals", "silicates", "ice", "stones", "labor"
};

std::array<double, Resource::eTotalResources> Resource::Density = {0};
const std::array<Resource::Type, 4> Resource::MaterialResources = {
  Resource::eMetal,
  Resource::eSilicate,
  Resource::eIce,
  Resource::eStone
};
const std::array<Resource::Type, 1> Resource::NonMaterialResources = {
  Resource::eLabor
};

bool Resource::initialize()
{
  Density[eMetal]    = 4500;  // Ti
  Density[eIce]      = 916;
  Density[eSilicate] = 2330;  // Si
  Density[eStone]    = Density[eSilicate];  // Useless silicates

  Density[eLabor]    = 0;     // It is non material resource

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
    std::make_pair(Names[eStone],    eStone),
    std::make_pair(Names[eLabor],    eLabor)
  };

  auto I = table.find(sType);
  assert(I != table.end());
  return I != table.end() ? I->second : eUnknown;
}

char const* Resource::typeToString(Resource::Type eType)
{
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

void ResourcesArray::dump(YAML::Node& out) const
{
  utils::YamlDumper dumper(out);
  for (Resource::Type eType : Resource::AllTypes) {
    if (utils::AlmostEqual(at(eType), 0))
      continue;
    dumper.add(Resource::typeToString(eType), at(eType));
  }
}

ResourcesArray& ResourcesArray::metals(double amount)
{
  at(Resource::eMetal) = amount;
  return *this;
}

ResourcesArray &ResourcesArray::silicates(double amount)
{
  at(Resource::eSilicate) = amount;
  return *this;
}

ResourcesArray &ResourcesArray::ice(double amount)
{
  at(Resource::eIce) = amount;
  return *this;
}

ResourcesArray& ResourcesArray::operator+=(ResourcesArray const& other)
{
  for (Resource::Type eType : Resource::AllTypes) {
    at(eType) += other.at(eType);
  }
  return *this;
}

bool ResourcesArray::operator==(ResourcesArray const& other) const
{
  for (Resource::Type eType : Resource::AllTypes) {
    if (!utils::AlmostEqual(at(eType), other.at(eType)))
      return false;
  }
  return true;
}

double ResourcesArray::calculateTotalVolume() const
{
  double volume = 0;
  for (Resource::Type eResource : Resource::MaterialResources) {
    volume += at(eResource) /  world::Resource::Density[eResource];
  }
  return volume;
}

} // namespace world
