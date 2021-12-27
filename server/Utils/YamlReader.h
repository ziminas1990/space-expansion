#pragma once

#include <stdint.h>
#include <string>
#include "YamlForwardDeclarations.h"

namespace geometry {
struct Point;
class  Vector;
} // namespace geometry

namespace world {
class ResourcesArray;
}

namespace utils {

// This reader supports only types, that are required, so it doesn't have any
// template functions in interface. It allows to include yaml.h ONLY in
// UtilsReader.cpp
class YamlReader
{
public:
  YamlReader(YAML::Node const& source);

  bool isOk() const { return noProblems; }
  operator bool() const { return noProblems; }

  // Built-in types:
  // TODO: replace with template?
  YamlReader& read(char const* pName, uint8_t& value);
  YamlReader& read(char const* pName, uint16_t& value);
  YamlReader& read(char const* pName, uint32_t& value);
  YamlReader& read(char const* pName, double& value);
  YamlReader& read(char const* pName, std::string& sValue);

  // Complex types for occasions
  YamlReader& read(char const* pName, geometry::Point& point);
  YamlReader& read(char const* pName, geometry::Vector& vector);
  YamlReader& read(char const* pName, world::ResourcesArray& resources);

private:
  YAML::Node const& source;
  bool noProblems;
};


} // namespace utils
