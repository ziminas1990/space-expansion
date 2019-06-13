#include "ClientCelestialScanner.h"

namespace autotests { namespace client {

bool CelestialScanner::getSpecification(CelestialScannerSpecification& specification)
{
  spex::Message request;
  request.mutable_celestialscanner()->mutable_get_specification();
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

}}  // namespace autotests::client
