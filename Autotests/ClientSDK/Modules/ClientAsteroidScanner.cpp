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
  specification.m_nProcessingTimeUs    = response.specification().scanning_time_k();
  return true;
}


}}  // namespace autotests::client
