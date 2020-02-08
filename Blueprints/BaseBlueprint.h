#pragma once

#include <memory>
#include <vector>

#include <Modules/BaseModule.h>
#include <Utils/YamlForwardDeclarations.h>
#include <World/Resources.h>

#include "BlueprintsLibrary.h"

namespace modules {

class BaseBlueprint;
using BaseBlueprintPtr = std::shared_ptr<BaseBlueprint>;

class BaseBlueprint
{
public:
  virtual ~BaseBlueprint() = default;

  virtual BaseModulePtr build(
      std::string sName, BlueprintsLibrary const& library) const = 0;

  virtual bool load(YAML::Node const& data) = 0;
  virtual void dump(YAML::Node& out) const = 0;

};

} // namespace modules
