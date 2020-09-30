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
public:
  AsteroidMiner(std::string sName, world::PlayerWeakPtr pOwner,
                uint32_t nMaxDistance, uint32_t nCycleTimeMs, uint32_t nYieldPerCycle);

  void proceed(uint32_t nIntervalUs) override;

private:
  void handleAsteroidMinerMessage(
      uint32_t nTunnelId, spex::IAsteroidMiner const& message) override;
  void yiedlAsteroidAndSendReport(world::Asteroid* pAsteroid);

  void bindToCargoRequest(uint32_t nTunnelId, std::string const& sCargoName);
  void startMiningRequest(uint32_t nTunnelId,
                          spex::IAsteroidMiner::MiningTask const& task);
  void stopMiningRequest(uint32_t nTunnelId);
  void onSpecificationRequest(uint32_t nTunnelId);

  world::Asteroid* getAsteroid(uint32_t nAsteroidId);
  bool isInRange(world::Asteroid* pAsteroid) const;

  void sendBindingStatus(uint32_t nTunnelId, spex::IAsteroidMiner::Status status);
  void sendStartMiningStatus(uint32_t nTunnelId, spex::IAsteroidMiner::Status status);
  void sendStopMiningStatus(uint32_t nTunnelId, spex::IAsteroidMiner::Status status);
  void sendMiningIsStopped(uint32_t nTunnelId, spex::IAsteroidMiner::Status status);

private:
  uint32_t    m_nMaxDistance;
  uint32_t    m_nCycleTimeMs;
  uint32_t    m_nYeildPerCycle;
  std::string m_sContainerName;

  ResourceContainerPtr   m_pContainer;
  uint32_t               m_nAsteroidId;
  uint64_t               m_nCycleProgressUs;
  uint32_t               m_nTunnelId;
  world::Resource::Type  m_eResourceType;
};

} // namespace modules
