#pragma once

#include <Blueprints/BaseBlueprint.h>
#include <Modules/AsteroidScanner/AsteroidScanner.h>
#include <Utils/YamlReader.h>
#include <Utils/YamlDumper.h>

namespace modules {

class AsteroidScannerBlueprint : public BaseBlueprint
{
public:
  AsteroidScannerBlueprint() : m_nMaxScanningDistance(0), m_nScanningTimeMs(0)
  {}

  BaseModulePtr build(std::string sName, BlueprintsLibrary const&) const override
  {
    return std::make_shared<AsteroidScanner>(
          std::move(sName), m_nMaxScanningDistance, m_nScanningTimeMs);
  }

  bool load(YAML::Node const& data) override
  {
    return utils::YamlReader(data)
        .read("max_scanning_distance", m_nMaxScanningDistance)
        .read("scanning_time_ms",      m_nScanningTimeMs);
  }

  void dump(YAML::Node& out) const override
  {
    utils::YamlDumper(out)
        .add("max_scanning_distance", m_nMaxScanningDistance)
        .add("scanning_time_ms",      m_nScanningTimeMs);
  }

private:
  uint32_t m_nMaxScanningDistance;
  uint32_t m_nScanningTimeMs;
};

} // namespace modules
