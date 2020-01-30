#pragma once

#include <memory>
#include <Modules/BaseModule.h>
#include <Utils/YamlForwardDeclarations.h>

namespace modules {

class ModuleBlueprint;
using ModuleBlueprintPtr = std::shared_ptr<ModuleBlueprint>;

class ModuleBlueprint
{
public:
  virtual ~ModuleBlueprint() = default;

  virtual BaseModulePtr build() const = 0;

  virtual bool load(YAML::Node const& data) = 0;
  virtual void dump(YAML::Node& out) const = 0;

};

} // namespace modules
