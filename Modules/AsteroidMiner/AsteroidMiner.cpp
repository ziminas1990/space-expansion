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
    case spex::ResourceType::ResourceType_INT_MAX_SENTINEL_DO_NOT_USE_:
    case spex::ResourceType::ResourceType_INT_MIN_SENTINEL_DO_NOT_USE_: {
      // to avoid warning
      assert(!"Unexpected resource type!");
    }
  }
  return world::Resources::eUnknown;
}


AsteroidMiner::AsteroidMiner(uint32_t nMaxDistance, uint32_t nCycleTimeMs)
  : BaseModule("AsteroidMiner", std::string()),
    m_nMaxDistance(nMaxDistance), m_nCycleTimeMs(nCycleTimeMs)
{
  GlobalContainer<AsteroidMiner>::registerSelf(this);
}

void AsteroidMiner::attachToResourceContainer(ResourceContainerPtr pContainer)
{
  m_pContainer = pContainer;
}

void AsteroidMiner::proceed(uint32_t nIntervalUs)
{

}

void AsteroidMiner::handleAsteroidMinerMessage(
    uint32_t nTunnelId, spex::IAsteroidMiner const& message)
{
  switch (message.choice_case()) {
    case spex::IAsteroidMiner::kStartMining: {

    } break;
    case spex::IAsteroidMiner::kStopMining: {

    } break;
    case spex::IAsteroidMiner::kSpecification: {
      onSpecificationRequest(nTunnelId);
    } break;
    default:
      return;
  }
}

void AsteroidMiner::startMiningRequest(uint32_t nTunnelId,
                                       spex::IAsteroidMiner::MiningTask const& task)
{
  if (m_eState != e_IDLE) {
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
  m_nCycleLeftUs  = m_nCycleTimeMs * 1000;
}

void AsteroidMiner::onSpecificationRequest(uint32_t nTunnelId)
{
  spex::Message response;
  spex::IAsteroidMiner::Specification* body =
      response.mutable_asteroid_miner()->mutable_specification();
  body->set_max_distance(m_nMaxDistance);
  body->set_cycle_time_ms(m_nCycleTimeMs);
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

} // namespace modules
