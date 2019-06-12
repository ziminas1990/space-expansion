#include "CelestialScanner.h"

#include <Ships/Ship.h>
#include <World/CelestialBodies/Asteroid.h>
#include <Utils/YamlReader.h>

#include <math.h>

namespace modules
{

CelestialScanner::CelestialScanner(
    uint32_t nMaxScanningRadiusKm, uint32_t nProcessingTimeUs)
  : BaseModule("CelestialScanner"),
    m_nMaxScanningRadiusKm(nMaxScanningRadiusKm), m_nProcessingTimeUs(nProcessingTimeUs)
{}

void CelestialScanner::proceed(uint32_t nIntervalUs)
{
  if (m_eState == State::eIdle)
    return;

  if (nIntervalUs < m_nScanningTimeLeftUs) {
    m_nScanningTimeLeftUs -= nIntervalUs;
    return;
  }

  collectAndSendScanResults();
  m_eState = State::eIdle;
}

void CelestialScanner::handleCelestialScannerMessage(
    uint32_t nTunnelId, spex::ICelestialScanner const& message)
{
  switch (message.choice_case())
  {
    case spex::ICelestialScanner::kScan: {
      onScanRequest(nTunnelId, message.scan().scanning_radius_km() * 1000,
                    message.scan().minimal_radius_m());
      return;
    }
    case spex::ICelestialScanner::kGetSpecification: {
      spex::Message response;
      spex::ICelestialScanner::Specification* pBody =
          response.mutable_celestialscanner()->mutable_specification();
      pBody->set_max_radius_km(m_nMaxScanningRadiusKm);
      pBody->set_processing_time_us(m_nProcessingTimeUs);
      sendToClient(nTunnelId, response);
      return;
    }
    default:
      return;
  }
}

void CelestialScanner::onScanRequest(
    uint32_t nTunnelId, uint32_t nScanningRadiusKm, uint32_t nMinimalRadius)
{
  if (m_eState != State::eIdle) {
    spex::Message busyResponse;
    busyResponse.mutable_celestialscanner()->mutable_busy_response();
    sendToClient(nTunnelId, busyResponse);
    return;
  }

  if (nScanningRadiusKm > m_nMaxScanningRadiusKm)
    nScanningRadiusKm = m_nMaxScanningRadiusKm;
  if (nMinimalRadius < 5)
    nMinimalRadius = 5;

  m_nScanningRadiusKm = nScanningRadiusKm;
  m_nMinimalRadius    = nMinimalRadius;
  m_nTunnelId         = nTunnelId;

  m_eState = State::eScanning;
  // ScanningTime = 100ms + 2 * RTT + ProcessingTime * (nScanningRadius/nMinimalRadius)
  // , where RTT = radius / c
  uint32_t RTT_Us       = nScanningRadiusKm * 10 / 3;
  uint32_t resolution   = nScanningRadiusKm * 1000 / nMinimalRadius;
  m_nScanningTimeLeftUs = 100000 + 2 * RTT_Us + resolution * m_nProcessingTimeUs;
}

void CelestialScanner::collectAndSendScanResults()
{
  std::vector<world::Asteroid*> const& allAsteroids =
      utils::GlobalContainer<world::Asteroid>::getAllInstancies();
  double maxRadiusSqr = 1000 * m_nScanningRadiusKm;
  maxRadiusSqr *= maxRadiusSqr;
  geometry::Point const& selfPosition = getPlatform()->getPosition();

  std::vector<world::Asteroid*> scannedAsteroids;
  scannedAsteroids.reserve(32);
  for (world::Asteroid* pAsteroid : allAsteroids) {
    if (!pAsteroid || pAsteroid->getRadius() < m_nMinimalRadius)
      continue;
    if (selfPosition.distanceSqr(pAsteroid->getPosition()) < maxRadiusSqr)
      scannedAsteroids.push_back(pAsteroid);
  }

  for (size_t i = 0; i < scannedAsteroids.size();)
  {
    spex::Message response;
    spex::ICelestialScanner::ScanResults *pBody =
        response.mutable_celestialscanner()->mutable_scan_result();
    auto pBatch = pBody->mutable_asteroids();
    for (size_t j = 0; j < 10 && i < scannedAsteroids.size(); ++j, ++i)
    {
      world::Asteroid* pAsteroid = scannedAsteroids[i];

      spex::ICelestialScanner::ScanResults::AsteroidInfo* pInfo = pBatch->Add();
      pInfo->set_id(pAsteroid->getAsteroidId());
      pInfo->set_x(pAsteroid->getPosition().x);
      pInfo->set_y(pAsteroid->getPosition().y);
      pInfo->set_vx(pAsteroid->getVelocity().getX());
      pInfo->set_vy(pAsteroid->getVelocity().getY());
      pInfo->set_r(pAsteroid->getRadius());
    }
    pBody->set_left(static_cast<uint32_t>(scannedAsteroids.size() - i));
    sendToClient(m_nTunnelId, response);
  }
}

} // namespace modules
