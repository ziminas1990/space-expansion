#include "AsteroidMiner.h"

#include <Ships/Ship.h>
#include <World/CelestialBodies/Asteroid.h>
#include <Utils/YamlReader.h>

#include <assert.h>
#include <math.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::AsteroidMiner);

namespace modules
{

static world::Resources::Type convert(spex::ResourceType eType)
{
  switch (eType) {
    case spex::ResourceType::RESOURCE_ICE:
      return world::Resources::eIce;
    case spex::ResourceType::RESOURCE_METTALS:
      return world::Resources::eMettal;
    case spex::ResourceType::RESOURCE_SILICATES:
      return world::Resources::eSilicate;
    case spex::ResourceType::RESOURCE_UNKNOWN:
    case spex::ResourceType::ResourceType_INT_MAX_SENTINEL_DO_NOT_USE_:
    case spex::ResourceType::ResourceType_INT_MIN_SENTINEL_DO_NOT_USE_: {
      // to avoid warning
      assert(nullptr == "Unexpected resource type!");
    }
  }
  return world::Resources::eUnknown;
}

static spex::ResourceType convert(world::Resources::Type eType)
{
  switch (eType) {
    case world::Resources::eIce:
      return spex::ResourceType::RESOURCE_ICE;
    case world::Resources::eMettal:
      return spex::ResourceType::RESOURCE_METTALS;
    case world::Resources::eSilicate:
      return spex::ResourceType::RESOURCE_SILICATES;
    case world::Resources::eTotalResources:
    case world::Resources::eUnknown: {
      // to avoid warning
      assert(nullptr == "Unexpected resource type!");
    }
  }
  return spex::ResourceType::RESOURCE_UNKNOWN;
}


AsteroidMiner::AsteroidMiner(uint32_t nMaxDistance, uint32_t nCycleTimeMs,
                             uint32_t nYieldPerCycle)
  : BaseModule("AsteroidMiner", std::string()),
    m_nMaxDistance(nMaxDistance), m_nCycleTimeMs(nCycleTimeMs),
    m_nYeildPerCycle(nYieldPerCycle), m_nCycleProgressUs(0), m_nTunnelId(0)
{
  GlobalContainer<AsteroidMiner>::registerSelf(this);
}

void AsteroidMiner::attachToResourceContainer(ResourceContainerPtr pContainer)
{
  m_pContainer = pContainer;
}

void AsteroidMiner::proceed(uint32_t nIntervalUs)
{
  world::Asteroid* pAsteroid = getAsteroid(m_nAsteroidId);
  if (!pAsteroid) {
    sendStartMiningStatus(m_nTunnelId, spex::IAsteroidMiner::ASTEROID_DOESNT_EXIST);
    onDeactivated();
    return;
  }

  if (!isInRange(pAsteroid)) {
    sendStartMiningStatus(m_nTunnelId, spex::IAsteroidMiner::ASTEROID_TOO_FAR);
    onDeactivated();
    return;
  }

  uint64_t nCycleTimeUs = m_nCycleTimeMs * 1000;
  m_nCycleProgressUs += nIntervalUs;
  if (m_nCycleProgressUs < nCycleTimeUs) {
    return;
  }

  m_nCycleProgressUs -= nCycleTimeUs;

  double percentage = pAsteroid->getComposition().percents[m_eResourceType];
  double amount = pAsteroid->yield(m_eResourceType, m_nYeildPerCycle * percentage);
  double put = m_pContainer->putResource(m_eResourceType, amount);

  spex::Message message;
  spex::IAsteroidMiner* pResponse = message.mutable_asteroid_miner();
  spex::ResourceItem* pItem = pResponse->mutable_mining_report();
  pItem->set_type(convert(m_eResourceType));
  pItem->set_amount(put);
  sendToClient(m_nTunnelId, message);

  if (put < amount) {
    sendError(m_nTunnelId, spex::IAsteroidMiner::NO_SPACE_AVALIABLE);
    onDeactivated();
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
    case spex::IAsteroidMiner::kSpecificationReq: {
      onSpecificationRequest(nTunnelId);
    } break;
    default:
      return;
  }
}

void AsteroidMiner::startMiningRequest(uint32_t nTunnelId,
                                       spex::IAsteroidMiner::MiningTask const& task)
{
  if (!isIdle()) {
    sendStartMiningStatus(nTunnelId, spex::IAsteroidMiner::MINER_IS_BUSY);
    return;
  }

  world::Asteroid* pAsteroid = getAsteroid(task.asteroid_id());
  if (!pAsteroid) {
    sendStartMiningStatus(nTunnelId, spex::IAsteroidMiner::ASTEROID_DOESNT_EXIST);
    return;
  }

  if (!isInRange(pAsteroid)) {
    sendStartMiningStatus(nTunnelId, spex::IAsteroidMiner::ASTEROID_TOO_FAR);
    return;
  }

  m_nAsteroidId   = task.asteroid_id();
  m_eResourceType = convert(task.resource());
  onActivated();
}

void AsteroidMiner::stopMiningRequest(uint32_t nTunnelId)
{
  if (!isActive()) {
    sendStopMiningStatus(nTunnelId, spex::IAsteroidMiner::MINER_IS_IDLE);
    return;
  }
  sendStopMiningStatus(nTunnelId, spex::IAsteroidMiner::SUCCESS);
  onDeactivated();
}

void AsteroidMiner::onSpecificationRequest(uint32_t nTunnelId)
{
  spex::Message response;
  spex::IAsteroidMiner::Specification* body =
      response.mutable_asteroid_miner()->mutable_specification();
  body->set_max_distance(m_nMaxDistance);
  body->set_cycle_time_ms(m_nCycleTimeMs);
  body->set_yeild_pre_cycle(m_nYeildPerCycle);
  sendToClient(nTunnelId, response);
}

world::Asteroid *AsteroidMiner::getAsteroid(uint32_t nAsteroidId)
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

void AsteroidMiner::sendStartMiningStatus(uint32_t nTunnelId,
                                          spex::IAsteroidMiner::Status status)
{
  spex::Message message;
  spex::IAsteroidMiner* pResponse = message.mutable_asteroid_miner();
  pResponse->set_stop_mining_status(status);
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

void AsteroidMiner::sendError(uint32_t nTunnelId, spex::IAsteroidMiner::Status error)
{
  spex::Message message;
  spex::IAsteroidMiner* pResponse = message.mutable_asteroid_miner();
  pResponse->set_on_error(error);
  sendToClient(nTunnelId, message);
}

} // namespace modules
