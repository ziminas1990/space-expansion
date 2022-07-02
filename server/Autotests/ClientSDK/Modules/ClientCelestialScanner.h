#pragma once

#include <Autotests/ClientSDK/Interfaces.h>
#include <Autotests/ClientSDK/ClientBaseModule.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <stdint.h>
#include <vector>

namespace autotests { namespace client {

struct CelestialScannerSpecification
{
  uint32_t m_nMaxScanningRadiusKm;
  uint32_t m_nProcessingTimeUs;
};

class CelestialScanner : public ClientBaseModule
{
public:
  struct AsteroidInfo
  {
    uint32_t         nId;
    geometry::Point  position;
    geometry::Vector velocity;
    double           radius;
  };

  bool getSpecification(CelestialScannerSpecification& specification);

  bool scan(uint32_t nRadiusKm, uint32_t nMinimalRadiusM,
            std::vector<AsteroidInfo>& asteroids);

};

}}  // namespace autotests::client
