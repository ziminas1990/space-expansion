#include "YamlReader.h"

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <yaml-cpp/yaml.h>

namespace utils
{

//========================================================================================
// helpers functions
//========================================================================================

template<typename T>
inline bool readSomeValue(YAML::Node const& node, char const* pName, T& value)
{
  YAML::Node const& container = node[pName];
  if (!container.IsDefined())
    return false;
  value = container.as<T>();
  return true;
}

//========================================================================================
// YamlReader
//========================================================================================

YamlReader::YamlReader(YAML::Node const& source)
  : source(source), noProblems(true)
{}

YamlReader &YamlReader::read(const char *pName, uint16_t &value)
{
  noProblems &= readSomeValue(source, pName, value);
  return *this;
}

YamlReader& YamlReader::read(const char *pName, uint32_t &value)
{
  noProblems &= readSomeValue(source, pName, value);
  return *this;
}

YamlReader& YamlReader::read(char const* pName, double& value)
{
  noProblems &= readSomeValue(source, pName, value);
  return *this;
}

YamlReader& YamlReader::read(char const* pName, std::string &sValue)
{
  noProblems &= readSomeValue(source, pName, sValue);
  return *this;
}

YamlReader& utils::YamlReader::read(char const* pName, geometry::Point& point)
{
  if (noProblems) {
    YAML::Node const& container = source[pName];
    noProblems &= container.IsDefined() && point.load(container);
  }
  return *this;
}

YamlReader& YamlReader::read(char const* pName, geometry::Vector& vector)
{
  if (noProblems) {
    YAML::Node const& container = source[pName];
    noProblems &= container.IsDefined() && vector.load(container);
  }
  return *this;
}

} // namespace utils
