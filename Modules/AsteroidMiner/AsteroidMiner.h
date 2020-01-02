#pragma once

#include <memory>
#include <Modules/BaseModule.h>
#include <Modules/ResourceContainer/ResourceContainer.h>
#include <Utils/GlobalContainer.h>
#include <Utils/YamlForwardDeclarations.h>
#include <World/Resources.h>
#include <Protocol.pb.h>

namespace world {
class Asteroid;
} // namespace world

namespace modules {

class AsteroidMiner :
    public BaseModule,
    public utils::GlobalContainer<AsteroidMiner>
{
  enum State {
    e_IDLE,
    e_MINING
  };

public:
  AsteroidMiner(uint32_t nMaxDistance, uint32_t nCycleTimeMs);

  void attachToResourceContainer(ResourceContainerPtr pContainer);

  void proceed(uint32_t nIntervalUs) override;

private:
  void handleAsteroidMinerMessage(
      uint32_t nTunnelId, spex::IAsteroidMiner const& message) override;

  void startMiningRequest(uint32_t nTunnelId,
                          spex::IAsteroidMiner::MiningTask const& task);
  void onSpecificationRequest(uint32_t nTunnelId);

  world::Asteroid* getAsteroid(uint32_t nAsteroidId);
  bool isInRange(world::Asteroid* pAsteroid) const;

  void sendStartMiningStatus(uint32_t nTunnelId, spex::IAsteroidMiner::Status status);
private:
  uint32_t m_nMaxDistance;
  uint32_t m_nCycleTimeMs;
  State    m_eState;

  ResourceContainerPtr   m_pContainer;
  uint32_t               m_nAsteroidId;
  uint64_t               m_nCycleLeftUs;
  world::Resources::Type m_eResourceType;
};

} // namespace modules
