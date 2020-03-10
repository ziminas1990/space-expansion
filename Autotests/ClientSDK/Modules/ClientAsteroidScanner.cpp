#include "ClientAsteroidScanner.h"

namespace autotests { namespace client {

static AsteroidScanner::Status convert(spex::IAsteroidScanner::Status eStatus) {
  switch (eStatus) {
    case spex::IAsteroidScanner::IN_PROGRESS:
      return AsteroidScanner::eInProgress;
    case spex::IAsteroidScanner::SCANNER_BUSY:
      return AsteroidScanner::eScannerIsBusy;
    case spex::IAsteroidScanner::ASTEROID_TOO_FAR:
      return AsteroidScanner::eAsteroidTooFar;
    default:
      assert(nullptr == "Unexpected status");
      return AsteroidScanner::eUnexpectedStatus;
  }
}

bool AsteroidScanner::getSpecification(AsteroidScannerSpecification &specification)
{
  spex::Message request;
  request.mutable_asteroid_scanner()->set_specification_req(true);
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
  request.mutable_asteroid_scanner()->set_scan_asteroid(nAsteroidId);
  if (!send(request))
    return eStatusError;

  spex::IAsteroidScanner response;
  // Waiting for 'IN PROGRESS' status message
  if (!wait(response))
    return eTimeoutError;
  if (response.choice_case() != spex::IAsteroidScanner::kScanningStatus)
    return eUnexpectedMessage;

  Status eStatus = convert(response.scanning_status());
  if (eStatus != eInProgress) {
    return eStatus;
  }

  // Waiting for scanning results
  if (!wait(response))
    return eTimeoutError;
  if (response.choice_case() != spex::IAsteroidScanner::kScanningFinished)
    return eStatusError;

  if (pResult) {
    spex::IAsteroidScanner::ScanResult const& scanResponse = response.scanning_finished();
    pResult->m_asteroidId       = scanResponse.asteroid_id();
    pResult->m_weight           = scanResponse.weight();
    pResult->m_metalsPercent    = scanResponse.metals_percent();
    pResult->m_silicatesPercent = scanResponse.silicates_percent();
    pResult->m_icePercent       = scanResponse.ice_percent();
  }
  return eSuccess;
}

}}  // namespace autotests::client
