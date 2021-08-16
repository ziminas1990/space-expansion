#include "AsteroidMiner.h"

#include <Ships/Ship.h>
#include <World/CelestialBodies/Asteroid.h>
#include <Utils/YamlReader.h>
#include <Utils/ItemsConverter.h>

#include <assert.h>
#include <math.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::AsteroidMiner);

namespace modules
{

AsteroidMiner::AsteroidMiner(std::string sName, world::PlayerWeakPtr pOwner,
                             uint32_t nMaxDistance, uint32_t nCycleTimeMs,
                             uint32_t nYieldPerCycle)
  : BaseModule("AsteroidMiner", std::move(sName), std::move(pOwner)),
    m_nMaxDistance(nMaxDistance), m_nCycleTimeMs(nCycleTimeMs),
    m_nYeildPerCycle(nYieldPerCycle),
    m_nCycleProgressUs(0), m_nTunnelId(0)
{
  GlobalContainer<AsteroidMiner>::registerSelf(this);
}

void AsteroidMiner::proceed(uint32_t nIntervalUs)
{
  getPlatform();

  world::Asteroid* pAsteroid = getAsteroid(m_nAsteroidId);
  if (!pAsteroid) {
    sendStartMiningStatus(m_nTunnelId, spex::IAsteroidMiner::ASTEROID_DOESNT_EXIST);
    switchToIdleState();
    return;
  }

  if (!isInRange(pAsteroid)) {
    sendStartMiningStatus(m_nTunnelId, spex::IAsteroidMiner::ASTEROID_TOO_FAR);
    switchToIdleState();
    return;
  }

  if (!m_pContainer) {
    sendStartMiningStatus(m_nTunnelId, spex::IAsteroidMiner::NOT_BOUND_TO_CARGO);
    switchToIdleState();
    return;
  }

  uint64_t nCycleTimeUs = m_nCycleTimeMs * 1000;
  m_nCycleProgressUs += nIntervalUs;
  if (m_nCycleProgressUs < nCycleTimeUs) {
    return;
  }
  m_nCycleProgressUs -= nCycleTimeUs;

  world::ResourcesArray mined = pAsteroid->yield(m_nYeildPerCycle);

  bool noSpaceLeft = false;

  spex::Message message;
  spex::IAsteroidMiner* pResponse = message.mutable_asteroid_miner();
  spex::Resources* pBody = pResponse->mutable_mining_report();
  for (world::Resource::Type eResource: world::Resource::MaterialResources) {
    if (eResource == world::Resource::eStone || mined[eResource] < 0.01) {
      continue;
    }
    double put = m_pContainer->putResource(eResource, mined[eResource]);
    spex::ResourceItem* pItem = pBody->add_items();
    pItem->set_type(utils::convert(eResource));
    pItem->set_amount(put);
    if (put < mined[eResource]) {
      noSpaceLeft = true;
      break;
    }
  }
  sendToClient(m_nTunnelId, message);
  if (noSpaceLeft) {
    sendMiningIsStopped(m_nTunnelId, spex::IAsteroidMiner::NO_SPACE_AVAILABLE);
    switchToIdleState();
  }
}

void AsteroidMiner::handleAsteroidMinerMessage(
    uint32_t nTunnelId, spex::IAsteroidMiner const& message)
{
  switch (message.choice_case()) {
    case spex::IAsteroidMiner::kStartMining: {
      startMiningRequest(nTunnelId, message.start_mining());
    } break;
    case spex::IAsteroidMiner::kStopMining: {
      stopMiningRequest(nTunnelId);
    } break;
    case spex::IAsteroidMiner::kBindToCargo: {
      bindToCargoRequest(nTunnelId, message.bind_to_cargo());
      break;
    }
    case spex::IAsteroidMiner::kSpecificationReq: {
      onSpecificationRequest(nTunnelId);
    } break;
    default:
      return;
  }
}

void AsteroidMiner::bindToCargoRequest(uint32_t nTunnelId, std::string const& sCargoName)
{
  BaseModulePtr pModule = getPlatform()->getModuleByName(sCargoName);
  ResourceContainerPtr pContainer = std::dynamic_pointer_cast<ResourceContainer>(pModule);
  if (!pContainer) {
    sendBindingStatus(nTunnelId, spex::IAsteroidMiner::NOT_BOUND_TO_CARGO);
    return;
  }
  m_pContainer = std::move(pContainer);
  m_sContainerName = sCargoName;
  sendBindingStatus(nTunnelId, spex::IAsteroidMiner::SUCCESS);
}

void AsteroidMiner::startMiningRequest(uint32_t nTunnelId, uint32_t asteroiId)
{
  if (!isIdle()) {
    sendStartMiningStatus(nTunnelId, spex::IAsteroidMiner::MINER_IS_BUSY);
    return;
  }

  if (!m_pContainer) {
    sendStartMiningStatus(nTunnelId, spex::IAsteroidMiner::NOT_BOUND_TO_CARGO);
    return;
  }

  world::Asteroid* pAsteroid = getAsteroid(asteroiId);
  if (!pAsteroid) {
    sendStartMiningStatus(nTunnelId, spex::IAsteroidMiner::ASTEROID_DOESNT_EXIST);
    return;
  }

  if (!isInRange(pAsteroid)) {
    sendStartMiningStatus(nTunnelId, spex::IAsteroidMiner::ASTEROID_TOO_FAR);
    return;
  }

  m_nAsteroidId   = asteroiId;
  m_nTunnelId     = nTunnelId;
  switchToActiveState();
  sendStartMiningStatus(nTunnelId, spex::IAsteroidMiner::SUCCESS);
}

void AsteroidMiner::stopMiningRequest(uint32_t nTunnelId)
{
  if (!isActive()) {
    sendStopMiningStatus(nTunnelId, spex::IAsteroidMiner::MINER_IS_IDLE);
    return;
  }
  sendStopMiningStatus(nTunnelId, spex::IAsteroidMiner::SUCCESS);
  sendMiningIsStopped(m_nTunnelId, spex::IAsteroidMiner::INTERRUPTED_BY_USER);
  switchToIdleState();
}

void AsteroidMiner::onSpecificationRequest(uint32_t nTunnelId)
{
  spex::Message response;
  spex::IAsteroidMiner::Specification* body =
      response.mutable_asteroid_miner()->mutable_specification();
  body->set_max_distance(m_nMaxDistance);
  body->set_cycle_time_ms(m_nCycleTimeMs);
  body->set_yield_per_cycle(m_nYeildPerCycle);
  sendToClient(nTunnelId, response);
}

world::Asteroid* AsteroidMiner::getAsteroid(uint32_t nAsteroidId)
{
  if (nAsteroidId >= world::AsteroidsContainer::TotalInstancies()) {
    return nullptr;
  }
  return world::AsteroidsContainer::Instance(nAsteroidId);
}

bool AsteroidMiner::isInRange(world::Asteroid *pAsteroid) const
{
  double distance = getPlatform()->getPosition().distance(pAsteroid->getPosition());
  return distance <= m_nMaxDistance;
}

void AsteroidMiner::sendBindingStatus(uint32_t nTunnelId,
                                      spex::IAsteroidMiner::Status status)
{
  assert(status == spex::IAsteroidMiner::SUCCESS ||
         status == spex::IAsteroidMiner::NOT_BOUND_TO_CARGO);
  spex::Message message;
  spex::IAsteroidMiner* pResponse = message.mutable_asteroid_miner();
  pResponse->set_bind_to_cargo_status(status);
  sendToClient(nTunnelId, message);
}

void AsteroidMiner::sendStartMiningStatus(uint32_t nTunnelId,
                                          spex::IAsteroidMiner::Status status)
{
  spex::Message message;
  spex::IAsteroidMiner* pResponse = message.mutable_asteroid_miner();
  pResponse->set_start_mining_status(status);
  sendToClient(nTunnelId, message);
}

void AsteroidMiner::sendStopMiningStatus(uint32_t nTunnelId,
                                         spex::IAsteroidMiner::Status status)
{
  spex::Message message;
  spex::IAsteroidMiner* pResponse = message.mutable_asteroid_miner();
  pResponse->set_stop_mining_status(status);
  sendToClient(nTunnelId, message);
}

void AsteroidMiner::sendMiningIsStopped(uint32_t nTunnelId,
                                        spex::IAsteroidMiner::Status status)
{
  spex::Message message;
  spex::IAsteroidMiner* pResponse = message.mutable_asteroid_miner();
  pResponse->set_mining_is_stopped(status);
  sendToClient(nTunnelId, message);
}

} // namespace modules
