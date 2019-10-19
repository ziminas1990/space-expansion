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

class AsteroidScanner :
    public BaseModule,
    public utils::GlobalContainer<AsteroidScanner>
{
public:
  AsteroidScanner(uint32_t nMaxDistance, uint32_t nScanningTimeMs);

  void proceed(uint32_t nIntervalUs) override;

private:
  void handleAsteroidScannerMessage(
      uint32_t nTunnelId, spex::IAsteroidScanner const& message) override;

  void onScanRequest(uint32_t nTunnelId, uint32_t nAsteroidId);

  world::Asteroid* getAndCheckAsteroid(uint32_t nAsteroidId);
  void sendFail(uint32_t nTunnelId);
private:
  uint32_t m_nMaxDistance;
  uint32_t m_nScanningTimeMs;

  uint64_t m_nScanningTimeLeftUs = 0;

  // Last scanning request parameters:
  uint32_t m_nAsteroidId = 0;
  uint32_t m_nTunnelId   = 0;
};

} // namespace modules
