#pragma once

#include <memory>
#include "BaseModule.h"

namespace modules {

class ModuleBlueprint;
using ModuleBlueprintPtr = std::shared_ptr<ModuleBlueprint>;

class ModuleBlueprint
{
public:
  virtual ~ModuleBlueprint() = default;

  virtual BaseModulePtr      build() const = 0;

  // Should a shared_ptr that stores a copy of current object
  virtual ModuleBlueprintPtr wrapToSharedPtr() = 0;
};

} // namespace modules
