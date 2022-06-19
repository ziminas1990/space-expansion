#include "CelestialScanner.h"

#include <Ships/Ship.h>
#include <Newton/PhysicalObject.h>
#include <World/CelestialBodies/Asteroid.h>
#include <World/Grid.h>
#include <Utils/YamlReader.h>

#include <math.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::CelestialScanner);

namespace modules
{

CelestialScanner::CelestialScanner(
    std::string &&sName, world::PlayerWeakPtr pOwner,
    uint32_t nMaxScanningRadiusKm, uint32_t nProcessingTimeUs)
  : BaseModule("CelestialScanner", std::move(sName), std::move(pOwner)),
    m_nMaxScanningRadiusKm(nMaxScanningRadiusKm), m_nProcessingTimeUs(nProcessingTimeUs)
{
  GlobalObject<CelestialScanner>::registerSelf(this);
}

void CelestialScanner::proceed(uint32_t nIntervalUs)
{
  if (nIntervalUs < m_nScanningTimeLeftUs) {
    m_nScanningTimeLeftUs -= nIntervalUs;
    return;
  }
  collectAndSendScanResults();
  switchToIdleState();
}

void CelestialScanner::onSessionClosed(uint32_t nSessionId)
{
  if (m_nTunnelId == nSessionId) {
    m_nTunnelId = 0;
    switchToIdleState();
  }
}

void CelestialScanner::handleCelestialScannerMessage(
    uint32_t nTunnelId, spex::ICelestialScanner const& message)
{
  switch (message.choice_case())
  {
    case spex::ICelestialScanner::kScan: {
      onScanRequest(nTunnelId, message.scan().scanning_radius_km(),
                    message.scan().minimal_radius_m());
      return;
    }
    case spex::ICelestialScanner::kSpecificationReq: {
      spex::Message response;
      spex::ICelestialScanner::Specification* pBody =
          response.mutable_celestial_scanner()->mutable_specification();
      pBody->set_max_radius_km(m_nMaxScanningRadiusKm);
      pBody->set_processing_time_us(m_nProcessingTimeUs);
      sendToClient(nTunnelId, std::move(response));
      return;
    }
    default:
      return;
  }
}

void CelestialScanner::onScanRequest(
    uint32_t nTunnelId, uint32_t nScanningRadiusKm, uint32_t nMinimalRadius)
{
  if (!isIdle()) {
    spex::Message busyResponse;
    busyResponse.mutable_celestial_scanner()->set_scanning_failed(
          spex::ICelestialScanner::SCANNER_BUSY);
    sendToClient(nTunnelId, std::move(busyResponse));
    return;
  }

  if (nScanningRadiusKm > m_nMaxScanningRadiusKm)
    nScanningRadiusKm = m_nMaxScanningRadiusKm;
  if (nMinimalRadius < 5)
    nMinimalRadius = 5;

  m_nScanningRadiusKm = nScanningRadiusKm;
  m_nMinimalRadius    = nMinimalRadius;
  m_nTunnelId         = nTunnelId;

  // ScanningTime = 100ms + 2 * RTT + ProcessingTime * (nScanningRadius/nMinimalRadius)
  // , where RTT = radius / c
  uint32_t RTT_Us       = nScanningRadiusKm * 10 / 3;
  uint32_t resolution   = nScanningRadiusKm * 1000 / nMinimalRadius;
  m_nScanningTimeLeftUs = 100000 + 2 * RTT_Us + resolution * m_nProcessingTimeUs;
  switchToActiveState();
}

void CelestialScanner::collectAndSendScanResults()
{
  geometry::Point const& selfPosition = getPlatform()->getPosition();
  world::Grid* pGrid = world::Grid::getGlobal();
  world::Grid::iterator itCell =
      pGrid->range<double>(
        selfPosition.x - m_nScanningRadiusKm * 1000,
        selfPosition.y - m_nScanningRadiusKm * 1000,
        2 * 1000 * m_nScanningRadiusKm,
        2 * 1000 * m_nScanningRadiusKm);

  double maxRadiusSqr = 1000 * m_nScanningRadiusKm;
  maxRadiusSqr *= maxRadiusSqr;

  std::vector<const world::Asteroid*> scannedAsteroids;
  scannedAsteroids.reserve(32);
  for (auto end = pGrid->end(); itCell != end; ++itCell) {
    for (const uint32_t nObjectId: itCell->getObjects().data()) {
      const newton::PhysicalObject* pObject =
          utils::GlobalContainer<newton::PhysicalObject>::Instance(nObjectId);
      if (pObject->is(world::ObjectType::eAsteroid)) {
        const world::Asteroid* pAsteroid =
            static_cast<const world::Asteroid*>(pObject);
        if (pAsteroid->getRadius() < m_nMinimalRadius) {
          continue;
        }
        if (selfPosition.distanceSqr(pAsteroid->getPosition()) < maxRadiusSqr)
          scannedAsteroids.push_back(pAsteroid);
      }
    }
  }

  if (scannedAsteroids.empty()) {
    spex::Message response;
    spex::ICelestialScanner::ScanResults *pBody =
        response.mutable_celestial_scanner()->mutable_scanning_report();
    pBody->set_left(0);
    sendToClient(m_nTunnelId, std::move(response));
    return;
  }

  for (size_t i = 0; i < scannedAsteroids.size();)
  {
    spex::Message response;
    spex::ICelestialScanner::ScanResults *pBody =
        response.mutable_celestial_scanner()->mutable_scanning_report();
    auto pBatch = pBody->mutable_asteroids();
    for (size_t j = 0; j < 10 && i < scannedAsteroids.size(); ++j, ++i)
    {
      const world::Asteroid* pAsteroid = scannedAsteroids[i];

      spex::ICelestialScanner::AsteroidInfo* pInfo = pBatch->Add();
      pInfo->set_id(pAsteroid->getAsteroidId());
      pInfo->set_x(pAsteroid->getPosition().x);
      pInfo->set_y(pAsteroid->getPosition().y);
      pInfo->set_vx(pAsteroid->getVelocity().getX());
      pInfo->set_vy(pAsteroid->getVelocity().getY());
      pInfo->set_r(pAsteroid->getRadius());
    }
    pBody->set_left(static_cast<uint32_t>(scannedAsteroids.size() - i));
    sendToClient(m_nTunnelId, std::move(response));
  }
}

} // namespace modules
