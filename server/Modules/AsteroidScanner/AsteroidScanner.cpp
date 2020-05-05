#include "AsteroidScanner.h"

#include <Ships/Ship.h>
#include <World/CelestialBodies/Asteroid.h>
#include <Utils/YamlReader.h>

#include <math.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::AsteroidScanner);

namespace modules
{

AsteroidScanner::AsteroidScanner(std::string&& sName, world::PlayerWeakPtr pOwner,
                                 uint32_t nMaxDistance, uint32_t nScanningTimeMs)
  : BaseModule("AsteroidScanner", std::move(sName), std::move(pOwner)),
    m_nMaxDistance(nMaxDistance), m_nScanningTimeMs(nScanningTimeMs)
{
  GlobalContainer<AsteroidScanner>::registerSelf(this);
}

void AsteroidScanner::proceed(uint32_t nIntervalUs)
{
  world::Asteroid* pAsteroid = getAndCheckAsteroid(m_nAsteroidId);
  if (!pAsteroid) {
    sendStatus(m_nTunnelId, spex::IAsteroidScanner::ASTEROID_TOO_FAR);
    switchToIdleState();
    return;
  }

  if (nIntervalUs < m_nScanningTimeLeftUs) {
    m_nScanningTimeLeftUs -= nIntervalUs;
    return;
  }

  spex::Message response;
  spex::IAsteroidScanner::ScanResult* pScanResult =
      response.mutable_asteroid_scanner()->mutable_scanning_finished();
  pScanResult->set_asteroid_id(m_nAsteroidId);
  pScanResult->set_weight(pAsteroid->getWeight());
  pScanResult->set_ice_percent(pAsteroid->getComposition().ice_percent());
  pScanResult->set_metals_percent(pAsteroid->getComposition().metals_percent());
  pScanResult->set_silicates_percent(pAsteroid->getComposition().silicates_percent());
  sendToClient(m_nTunnelId, std::move(response));
  m_nTunnelId = 0;
  switchToIdleState();
}

void AsteroidScanner::handleAsteroidScannerMessage(
    uint32_t nTunnelId, spex::IAsteroidScanner const& message)
{
  switch (message.choice_case())
  {
    case spex::IAsteroidScanner::kScanAsteroid: {
      onScanRequest(nTunnelId, message.scan_asteroid());
      return;
    }
    case spex::IAsteroidScanner::kSpecificationReq: {
      spex::Message response;
      spex::IAsteroidScanner::Specification* pBody =
          response.mutable_asteroid_scanner()->mutable_specification();
      pBody->set_max_distance(m_nMaxDistance);
      pBody->set_scanning_time_ms(m_nScanningTimeMs);
      sendToClient(nTunnelId, response);
      return;
    }
    default:
      return;
  }
}

void AsteroidScanner::onScanRequest(uint32_t nTunnelId, uint32_t nAsteroidId)
{
  if (!isIdle()) {
    sendStatus(nTunnelId, spex::IAsteroidScanner::SCANNER_BUSY);
    return;
  }

  world::Asteroid* pAsteroid = getAndCheckAsteroid(nAsteroidId);
  if (!pAsteroid) {
    sendStatus(nTunnelId, spex::IAsteroidScanner::ASTEROID_TOO_FAR);
    return;
  }

  double surfaceHectare = 4 * M_PI * pow(pAsteroid->getRadius(), 2) / 10000.0;
  m_nScanningTimeLeftUs = static_cast<uint64_t>(surfaceHectare * m_nScanningTimeMs * 1000);
  m_nAsteroidId         = nAsteroidId;
  m_nTunnelId           = nTunnelId;
  switchToActiveState();
  sendStatus(nTunnelId, spex::IAsteroidScanner::IN_PROGRESS);
}

world::Asteroid* AsteroidScanner::getAndCheckAsteroid(uint32_t nAsteroidId)
{
  if (nAsteroidId >= world::AsteroidsContainer::TotalInstancies()) {
    return nullptr;
  }
  world::Asteroid* pAsteroid = world::AsteroidsContainer::Instance(nAsteroidId);
  if(!pAsteroid) {
    return nullptr;
  }
  double distance = getPlatform()->getPosition().distance(pAsteroid->getPosition());
  if (distance > m_nMaxDistance) {
    return nullptr;
  }
  return pAsteroid;
}

void AsteroidScanner::sendStatus(uint32_t nTunnelId,
                                 spex::IAsteroidScanner::Status status)
{
  spex::Message statusMessage;
  statusMessage.mutable_asteroid_scanner()->set_scanning_status(status);
  sendToClient(nTunnelId, statusMessage);
}


} // namespace modules
