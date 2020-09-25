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
    onDeactivated();
    return;
  }

  if (!isInRange(pAsteroid)) {
    sendStartMiningStatus(m_nTunnelId, spex::IAsteroidMiner::ASTEROID_TOO_FAR);
    onDeactivated();
    return;
  }

  if (!m_pContainer) {
    sendStartMiningStatus(m_nTunnelId, spex::IAsteroidMiner::NOT_BINT_TO_CARGO);
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
  double amount     = pAsteroid->yield(m_eResourceType, m_nYeildPerCycle * percentage);
  double put        = m_pContainer->putResource(m_eResourceType, amount);

  spex::Message message;
  spex::IAsteroidMiner* pResponse = message.mutable_asteroid_miner();
  spex::ResourceItem* pItem = pResponse->mutable_mining_report();
  pItem->set_type(utils::convert(m_eResourceType));
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
  m_sContainerName = sCargoName;

  BaseModulePtr pModule = getPlatform()->getModuleByName(m_sContainerName);
  ResourceContainerPtr pContainer = std::dynamic_pointer_cast<ResourceContainer>(pModule);
  if (!pContainer) {
    sendBindingStatus(nTunnelId, spex::IAsteroidMiner::NOT_BINT_TO_CARGO);
    return;
  }
  m_pContainer = std::move(pContainer);
  sendBindingStatus(nTunnelId, spex::IAsteroidMiner::SUCCESS);
}

void AsteroidMiner::startMiningRequest(uint32_t nTunnelId,
                                       spex::IAsteroidMiner::MiningTask const& task)
{
  if (!isIdle()) {
    sendStartMiningStatus(nTunnelId, spex::IAsteroidMiner::MINER_IS_BUSY);
    return;
  }

  if (!m_pContainer) {
    sendStartMiningStatus(nTunnelId, spex::IAsteroidMiner::NOT_BINT_TO_CARGO);
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
  m_nTunnelId     = nTunnelId;
  m_eResourceType = utils::convert(task.resource());
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
  switchToIdleState();
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

void AsteroidMiner::sendBindingStatus(uint32_t nTunnelId,
                                      spex::IAsteroidMiner::Status status)
{
  assert(status == spex::IAsteroidMiner::SUCCESS ||
         status == spex::IAsteroidMiner::NOT_BINT_TO_CARGO);
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

void AsteroidMiner::sendError(uint32_t nTunnelId, spex::IAsteroidMiner::Status error)
{
  spex::Message message;
  spex::IAsteroidMiner* pResponse = message.mutable_asteroid_miner();
  pResponse->set_on_error(error);
  sendToClient(nTunnelId, message);
}

} // namespace modules
