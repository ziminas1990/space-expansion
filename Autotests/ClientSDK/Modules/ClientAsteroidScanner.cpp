#include "ClientAsteroidScanner.h"

namespace autotests { namespace client {

bool AsteroidScanner::getSpecification(AsteroidScannerSpecification &specification)
{
  spex::Message request;
  request.mutable_asteroid_scanner()->mutable_get_specification();
  if (!send(request))
    return false;

  spex::IAsteroidScanner response;
  if (!wait(response))
    return false;
  if (response.choice_case() != spex::IAsteroidScanner::kSpecification)
    return false;

  specification.m_nMaxScanningDistance = response.specification().max_distance();
  specification.m_nProcessingTimeUs    = response.specification().scanning_time_ms();
  return true;
}

AsteroidScanner::Status
AsteroidScanner::scan(uint32_t nAsteroidId, AsteroidScanner::AsteroidInfo *pResult)
{
  spex::Message request;
  spex::IAsteroidScanner::ScanRequest* pScanRequest =
      request.mutable_asteroid_scanner()->mutable_scan_request();
  pScanRequest->set_asteroid_id(nAsteroidId);
  if (!send(request))
    return eStatusError;

  spex::IAsteroidScanner response;
  if (!wait(response))
    return eStatusError;
  if (response.choice_case() == spex::IAsteroidScanner::kScanFailed)
    return eStatusScanFailed;
  if (response.choice_case() != spex::IAsteroidScanner::kScanResult)
    return eStatusError;

  if (pResult) {
    spex::IAsteroidScanner::ScanResult const& scanResponse = response.scan_result();
    pResult->m_asteroidId       = scanResponse.asteroid_id();
    pResult->m_weight           = scanResponse.weight();
    pResult->m_metalsPercent    = scanResponse.metals_percent();
    pResult->m_silicatesPercent = scanResponse.silicates_percent();
    pResult->m_icePercent       = scanResponse.ice_percent();
  }
  return eStatusOk;
}

}}  // namespace autotests::client
