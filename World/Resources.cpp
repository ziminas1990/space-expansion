#include "Resources.h"

#include <assert.h>
#include <map>
#include <utility>

#include <yaml-cpp/yaml.h>

#include <Utils/FloatComparator.h>

namespace world {

std::vector<double> Resource::density;

bool Resource::initialize()
{
  density.resize(eTotalResources);
  density[eMetal]    = 4500;  // Ti
  density[eIce]      = 916;
  density[eSilicate] = 2330;  // Si
  density[eLabor]    = 0;     // It non material resource
  return true;
}

Resource::Type Resource::typeFromString(std::string const& sType)
{
  const static std::map<std::string, Type> table = {
    std::make_pair("metal",    eMetal),
    std::make_pair("ice",      eIce),
    std::make_pair("silicate", eSilicate),
    std::make_pair("labor",    eLabor)
  };

  auto I = table.find(sType);
  assert(I != table.end());
  return I != table.end() ? I->second : eUnknown;
}

std::string const& Resource::typeToString(Resource::Type eType)
{
  const static std::string unknown("unknown");
  const static std::map<Type, std::string> table = {
    std::make_pair(eMetal,    "metal"),
    std::make_pair(eIce,      "ice"),
    std::make_pair(eSilicate, "silicate"),
    std::make_pair(eLabor,    "labor")
  };

  auto I = table.find(eType);
  assert(I != table.end());
  return I != table.end() ? I->second : unknown;
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

} // namespace world
