#pragma once

#include <memory>
#include <Modules/BaseModule.h>
#include <Utils/YamlForwardDeclarations.h>

#include "BlueprintsLibrary.h"

namespace modules {

class AbstractBlueprint;
using AbstractBlueprintPtr = std::shared_ptr<AbstractBlueprint>;

class AbstractBlueprint
{
public:
  virtual ~AbstractBlueprint() = default;

  virtual BaseModulePtr build(
      std::string sName, BlueprintsLibrary const& library) const = 0;

  virtual bool load(YAML::Node const& data) = 0;
  virtual void dump(YAML::Node& out) const = 0;

};

} // namespace modules
