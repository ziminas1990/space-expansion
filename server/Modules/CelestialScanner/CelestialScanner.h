#pragma once

#include <memory>
#include <Modules/BaseModule.h>
#include <Utils/GlobalContainer.h>
#include <Utils/YamlForwardDeclarations.h>
#include <Protocol.pb.h>

namespace world {
class Asteroid;
} // namespace world

namespace modules {

class CelestialScanner :
    public BaseModule,
    public utils::GlobalContainer<CelestialScanner>
{
public:
  CelestialScanner(std::string&& sName, world::PlayerWeakPtr pOwner,
                   uint32_t m_nMaxScanningRadiusKm, uint32_t m_nProcessingTimeUs);

  void proceed(uint32_t nIntervalUs) override;

private:
  void handleCelestialScannerMessage(
      uint32_t nTunnelId, spex::ICelestialScanner const& message) override;

  void onScanRequest(uint32_t nTunnelId, uint32_t nScanningRadiusKm,
                     uint32_t nMinimalRadius);

  void collectAndSendScanResults();

private:
  uint32_t m_nMaxScanningRadiusKm;
  uint32_t m_nProcessingTimeUs;

  uint64_t m_nScanningTimeLeftUs = 0;

  // Last scanning request parameters:
  uint32_t m_nScanningRadiusKm = 0;
  uint32_t m_nMinimalRadius = 0;
  uint32_t m_nTunnelId      = 0;
};

} // namespace modules
