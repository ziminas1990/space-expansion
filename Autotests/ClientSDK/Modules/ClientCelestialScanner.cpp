#include "ClientCelestialScanner.h"

namespace autotests { namespace client {

bool CelestialScanner::getSpecification(CelestialScannerSpecification& specification)
{
  spex::Message request;
  request.mutable_celestialscanner()->set_specification_req(true);
  if (!send(request))
    return false;

  spex::ICelestialScanner response;
  if (!wait(response))
    return false;
  if (response.choice_case() != spex::ICelestialScanner::kSpecification)
    return false;

  specification.m_nMaxScanningRadiusKm = response.specification().max_radius_km();
  specification.m_nProcessingTimeUs    = response.specification().processing_time_us();
  return true;
}

bool CelestialScanner::scan(uint32_t nRadiusKm,
                            uint32_t nMinimalRadiusM,
                            std::vector<AsteroidInfo>& asteroids)
{
  spex::Message request;
  spex::ICelestialScanner::Scan* pBody = request.mutable_celestialscanner()->mutable_scan();
  pBody->set_scanning_radius_km(nRadiusKm);
  pBody->set_minimal_radius_m(nMinimalRadiusM);
  if (!send(request))
    return false;

  size_t   nTotalAsteroids = 0;
  uint32_t nAsteroidsLeft = 0;
  do {
    spex::ICelestialScanner response;
    if (!wait(response))
      return false;
    if (response.choice_case() != spex::ICelestialScanner::kScanningReport)
      return false;

    nAsteroidsLeft = response.scanning_report().left();

    if (!nTotalAsteroids) {
      nTotalAsteroids = static_cast<size_t>(nAsteroidsLeft) +
          static_cast<size_t>(response.scanning_report().asteroids().size());
      asteroids.reserve(nTotalAsteroids);
    }

    for (auto const& asteroid : response.scanning_report().asteroids())
    {
      AsteroidInfo info;
      info.nId      = asteroid.id();
      info.radius   = asteroid.r();
      info.position = geometry::Point(asteroid.x(), asteroid.y());
      info.velocity = geometry::Vector(asteroid.vx(), asteroid.vy());
      asteroids.push_back(std::move(info));
    }
  } while(nAsteroidsLeft);
  return true;
}

}}  // namespace autotests::client
