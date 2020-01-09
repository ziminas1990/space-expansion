#pragma once

#include "Containers.h"
#include <Utils/YamlForwardDeclarations.h>

namespace config
{

class PortsPoolCfgReader
{
public:
  static PortsPoolCfg read(YAML::Node const& data);
};

class ApplicationCfgReader
{
public:
  static ApplicationCfg read(YAML::Node const& data);
};

} // namespace config
