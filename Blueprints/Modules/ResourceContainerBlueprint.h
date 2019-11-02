#pragma once

#include "ModuleBlueprint.h"
#include <Modules/ResourceContainer/ResourceContainer.h>

namespace modules {

class ResourceContainerBlueprint : public ModuleBlueprint
{
public:
  ResourceContainerBlueprint() : m_nVolume(0)
  {}

  ResourceContainerBlueprint& setVolume(uint32_t nVolume)
  {
    m_nVolume = nVolume;
    return *this;
  }

  BaseModulePtr build() const override
  {
    return std::make_shared<ResourceContainer>(m_nVolume);
  }

  ModuleBlueprintPtr wrapToSharedPtr() override
  {
    return std::make_shared<ResourceContainerBlueprint>(std::move(*this));
  }

private:
  uint32_t m_nVolume;
};

} // namespace modules
