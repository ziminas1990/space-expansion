#pragma once

#include <Blueprints/BaseBlueprint.h>
#include <Modules/ResourceContainer/ResourceContainer.h>
#include <Utils/YamlDumper.h>
#include <Utils/YamlReader.h>

namespace blueprints {

class ResourceContainerBlueprint : public BaseBlueprint
{
public:
  ResourceContainerBlueprint() : m_nVolume(0)
  {}

  modules::BaseModulePtr
  build(std::string sName, world::PlayerWeakPtr pOwner) const override
  {
    return std::make_shared<modules::ResourceContainer>(
          std::move(sName), std::move(pOwner), m_nVolume);
  }

  bool load(YAML::Node const& data) override
  {
    return BaseBlueprint::load(data)
        && utils::YamlReader(data).read("volume", m_nVolume);
  }

  void dump(YAML::Node& out) const override
  {
    BaseBlueprint::dump(out);
    utils::YamlDumper(out).add("volume", m_nVolume);
  }

private:
  uint32_t m_nVolume;
};

} // namespace modules
