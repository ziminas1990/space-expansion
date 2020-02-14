#pragma once

#include <Blueprints/BaseBlueprint.h>
#include <Modules/AsteroidScanner/AsteroidScanner.h>
#include <Utils/YamlReader.h>
#include <Utils/YamlDumper.h>

namespace blueprints {

class AsteroidScannerBlueprint : public BaseBlueprint
{
public:
  AsteroidScannerBlueprint() : m_nMaxScanningDistance(0), m_nScanningTimeMs(0)
  {}

  modules::BaseModulePtr
  build(std::string sName, world::PlayerWeakPtr pOwner) const override
  {
    return std::make_shared<modules::AsteroidScanner>(
          std::move(sName), std::move(pOwner), m_nMaxScanningDistance, m_nScanningTimeMs);
  }

  bool load(YAML::Node const& data) override
  {
    return BaseBlueprint::load(data)
        && utils::YamlReader(data)
           .read("max_scanning_distance", m_nMaxScanningDistance)
           .read("scanning_time_ms",      m_nScanningTimeMs);
  }

  void dump(YAML::Node& out) const override
  {
    BaseBlueprint::dump(out);
    utils::YamlDumper(out)
        .add("max_scanning_distance", m_nMaxScanningDistance)
        .add("scanning_time_ms",      m_nScanningTimeMs);
  }

private:
  uint32_t m_nMaxScanningDistance;
  uint32_t m_nScanningTimeMs;
};

} // namespace modules
