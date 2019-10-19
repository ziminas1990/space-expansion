#include "AsteroidScanner.h"

#include <Ships/Ship.h>
#include <World/CelestialBodies/Asteroid.h>
#include <Utils/YamlReader.h>

#include <math.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::AsteroidScanner);

namespace modules
{

AsteroidScanner::AsteroidScanner(uint32_t nMaxDistance, uint32_t nScanningTimeMs)
  : BaseModule("AsteroidScanner"),
    m_nMaxDistance(nMaxDistance), m_nScanningTimeMs(nScanningTimeMs)
{
  GlobalContainer<AsteroidScanner>::registerSelf(this);
}

void AsteroidScanner::proceed(uint32_t nIntervalUs)
{
  world::Asteroid* pAsteroid = getAndCheckAsteroid(m_nAsteroidId);
  if (!pAsteroid) {
    sendFail(m_nTunnelId);
    switchToIdleState();
    return;
  }

  if (nIntervalUs < m_nScanningTimeLeftUs) {
    m_nScanningTimeLeftUs -= nIntervalUs;
    return;
  }

  spex::Message response;
  spex::IAsteroidScanner::ScanResult* pScanResult =
      response.mutable_asteroid_scanner()->mutable_scan_result();
  pScanResult->set_asteroid_id(m_nAsteroidId);
  pScanResult->set_weight(pAsteroid->getWeight());
  pScanResult->set_ice_percent(pAsteroid->getComposition().nIce);
  pScanResult->set_metals_percent(pAsteroid->getComposition().nMettals);
  pScanResult->set_silicates_percent(pAsteroid->getComposition().nSilicates);
  sendToClient(m_nTunnelId, std::move(response));
  switchToIdleState();
}

void AsteroidScanner::handleAsteroidScannerMessage(
    uint32_t nTunnelId, spex::IAsteroidScanner const& message)
{
  switch (message.choice_case())
  {
    case spex::IAsteroidScanner::kScanRequest: {
      onScanRequest(nTunnelId, message.scan_request().asteroid_id());
      return;
    }
    case spex::IAsteroidScanner::kGetSpecification: {
      spex::Message response;
      spex::IAsteroidScanner::Specification* pBody =
          response.mutable_asteroid_scanner()->mutable_specification();
      pBody->set_max_distance(m_nMaxDistance);
      pBody->set_scanning_time_k(m_nScanningTimeMs);
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
    sendFail(nTunnelId);
    return;
  }

  world::Asteroid* pAsteroid = getAndCheckAsteroid(nAsteroidId);
  if (!pAsteroid) {
    sendFail(nTunnelId);
    return;
  }

  double surfaceKm2     = 4 * M_PI * pow(pAsteroid->getRadius() / 1000.0, 2);
  m_nScanningTimeLeftUs = surfaceKm2 * m_nScanningTimeMs * 1000;
  m_nAsteroidId         = nAsteroidId;
  switchToActiveState();
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

void AsteroidScanner::sendFail(uint32_t nTunnelId)
{
  spex::Message busyResponse;
  busyResponse.mutable_asteroid_scanner()->mutable_scan_failed();
  sendToClient(nTunnelId, busyResponse);
}


} // namespace modules
