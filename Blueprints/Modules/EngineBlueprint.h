#pragma once

#include <Blueprints/Modules/ModuleBlueprint.h>
#include <Modules/Engine/Engine.h>
#include <Utils/YamlDumper.h>
#include <Utils/YamlReader.h>

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

  bool load(YAML::Node const& data) override
  {
    return utils::YamlReader(data)
        .read("max_thrust", m_nMaxThrust);
  }

  void dump(YAML::Node& out) const override
  {
    utils::YamlDumper(out)
            .add("max_thrust", m_nMaxThrust);
  }

  BaseModulePtr build() const override
  {
    return std::make_shared<Engine>(m_nMaxThrust);
  }

private:
  uint32_t m_nMaxThrust;
};

} // namespace modules
