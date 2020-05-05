#pragma once

#include <Autotests/ClientSDK/Interfaces.h>
#include <Autotests/ClientSDK/ClientBaseModule.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <stdint.h>
#include <vector>

namespace autotests { namespace client {

struct AsteroidScannerSpecification
{
  uint32_t m_nMaxScanningDistance;
  uint32_t m_nProcessingTimeUs;
};

class AsteroidScanner : public ClientBaseModule
{
public:
  enum Status {
    eSuccess,

    // Status, that are got from module
    eInProgress,
    eScannerIsBusy,
    eAsteroidTooFar,

    // Errors, detected on client side:
    eTransportError,
    eUnexpectedMessage,
    eUnexpectedStatus,
    eTimeoutError,
    eStatusError
  };

  struct AsteroidInfo
  {
    uint32_t m_asteroidId;
    double   m_weight;
    double   m_metalsPercent;
    double   m_icePercent;
    double   m_silicatesPercent;
  };

  bool getSpecification(AsteroidScannerSpecification& specification);

  Status scan(uint32_t nAsteroidId, AsteroidInfo* pResult = nullptr);
};

using AsteroidScannerPtr = std::shared_ptr<AsteroidScanner>;

}}  // namespace autotests::client
