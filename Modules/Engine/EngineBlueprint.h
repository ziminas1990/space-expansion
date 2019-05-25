#pragma once

#include <Modules/ModuleBlueprint.h>
#include "Engine.h"

namespace modules {

class EngineBlueprint : public ModuleBlueprint
{
public:

  EngineBlueprint() : m_nMaxThrust(0) {}

  EngineBlueprint& setMaxThrust(uint32_t nMaxThrust)
  {
    m_nMaxThrust = nMaxThrust;
    return *this;
  }

  BaseModulePtr build() const override
  {
    return std::make_shared<Engine>(m_nMaxThrust);
  }

  ModuleBlueprintPtr wrapToSharedPtr() override
  {
    return std::make_shared<EngineBlueprint>(std::move(*this));
  }

private:
  uint32_t m_nMaxThrust;
};

} // namespace modules
