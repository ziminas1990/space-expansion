#pragma once

#include <Autotests/ClientSDK/Interfaces.h>
#include <Autotests/ClientSDK/ClientBaseModule.h>
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

  bool getSpecification(CelestialScannerSpecification& specification);

};

using CelestialScannerPtr = std::shared_ptr<CelestialScanner>;

}}  // namespace autotests::client
