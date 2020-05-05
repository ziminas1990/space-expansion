#pragma once

#include <Blueprints/BaseBlueprint.h>
#include <Modules/Engine/Engine.h>
#include <Utils/YamlDumper.h>
#include <Utils/YamlReader.h>

namespace blueprints {

class EngineBlueprint : public BaseBlueprint
{
public:

  EngineBlueprint() : m_nMaxThrust(0) {}

  modules::BaseModulePtr
  build(std::string sName, world::PlayerWeakPtr pOwner) const override
  {
    return std::make_shared<modules::Engine>(
          std::move(sName), std::move(pOwner), m_nMaxThrust);
  }

  bool load(YAML::Node const& data) override
  {
    return BaseBlueprint::load(data)
        && utils::YamlReader(data).read("max_thrust", m_nMaxThrust);
  }

  void dump(YAML::Node& out) const override
  {
    BaseBlueprint::dump(out);
    utils::YamlDumper(out)
            .add("max_thrust", m_nMaxThrust);
  }

private:
  uint32_t m_nMaxThrust;
};

} // namespace modules
