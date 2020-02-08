#pragma once

#include <Blueprints/AbstractBlueprint.h>
#include <Modules/Engine/Engine.h>
#include <Utils/YamlDumper.h>
#include <Utils/YamlReader.h>

namespace modules {

class EngineBlueprint : public AbstractBlueprint
{
public:

  EngineBlueprint() : m_nMaxThrust(0) {}

  BaseModulePtr build(std::string sName, BlueprintsLibrary const&) const override
  {
    return std::make_shared<Engine>(std::move(sName), m_nMaxThrust);
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

private:
  uint32_t m_nMaxThrust;
};

} // namespace modules
