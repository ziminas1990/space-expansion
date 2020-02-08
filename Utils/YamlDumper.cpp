#include "YamlDumper.h"
#include <yaml-cpp/yaml.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>

namespace utils {

YamlDumper& YamlDumper::add(char const* pName, uint16_t value)
{
  m_data[pName] = value;
  return *this;
}

YamlDumper& YamlDumper::add(char const* pName, uint32_t value)
{
  m_data[pName] = value;
  return *this;
}

YamlDumper& YamlDumper::add(char const* pName, double value)
{
  m_data[pName] = value;
  return *this;
}

YamlDumper& YamlDumper::add(char const* pName, std::string const& sValue)
{
  m_data[pName] = sValue;
  return *this;
}

YamlDumper& YamlDumper::add(char const* pName, geometry::Point const& point)
{
  YAML::Node node;
  point.dump(node);
  m_data[pName] = node;
  return *this;
}

YamlDumper& YamlDumper::add(char const* pName, geometry::Vector const& vector)
{
  YAML::Node node;
  vector.dump(node);
  m_data[pName] = node;
  return *this;
}

YamlDumper &YamlDumper::add(const char *pName, YAML::Node &&node)
{
  m_data[pName] = std::move(node);
  return *this;
}



} // namespace utils
