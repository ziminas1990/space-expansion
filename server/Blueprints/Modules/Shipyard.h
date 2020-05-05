#pragma once

#include <Blueprints/BaseBlueprint.h>
#include <Modules/Shipyard/Shipyard.h>
#include <Utils/YamlDumper.h>
#include <Utils/YamlReader.h>

namespace blueprints {

class ShipyardBlueprint : public BaseBlueprint
{
public:
  ShipyardBlueprint() : m_nLaborPerSec(0)
  {}

  modules::BaseModulePtr
  build(std::string sName, world::PlayerWeakPtr pOwner) const override
  {
    return std::make_shared<modules::Shipyard>(
          std::move(sName),
          std::move(pOwner),
          m_nLaborPerSec,
          std::move(m_sContainerName));
  }

  bool load(YAML::Node const& data) override
  {
    return BaseBlueprint::load(data)
        && utils::YamlReader(data)
           .read("productivity",   m_nLaborPerSec)
           .read("container_name", m_sContainerName);
  }

  void dump(YAML::Node& out) const override
  {
    BaseBlueprint::dump(out);
    utils::YamlDumper(out)
        .add("productivity",   m_nLaborPerSec)
        .add("container_name", m_sContainerName);
  }

private:
  uint32_t    m_nLaborPerSec;
  std::string m_sContainerName;
};

} // namespace modules
