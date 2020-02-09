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

  virtual bool load(YAML::Node const& data);
  virtual void dump(YAML::Node& out) const;

  world::Resources const& cost()  const { return m_expenses; }
    // Return total number of resources, that are required to produce item

private:
  world::Resources m_expenses;
};

} // namespace modules
