#pragma once

#include <stdint.h>
#include <string>
#include <Utils/YamlForwardDeclarations.h>

namespace geometry
{
struct Point;
class  Vector;
} // namespace geometry

namespace utils {

// This dumper supports only types, that are required, so, it doesn't have any template
// functions in interface. It allows to inclue yaml.h ONLY in UtilsReader.cpp
class YamlDumper
{
public:
  YamlDumper(YAML::Node& node) : m_data(node) {}

  // Built-in types:
  // TODO: replace with template?
  YamlDumper& add(char const* pName, uint16_t value);
  YamlDumper& add(char const* pName, uint32_t value);
  YamlDumper& add(char const* pName, double value);
  YamlDumper& add(char const* pName, std::string const& sValue);

  // Complex types:
  YamlDumper& add(char const* pName, geometry::Point const& point);
  YamlDumper& add(char const* pName, geometry::Vector const& vector);
  YamlDumper& add(char const* pName, YAML::Node&& node);
private:
  YAML::Node& m_data;
};


} // namespace utils
