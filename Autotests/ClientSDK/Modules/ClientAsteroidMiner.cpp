#include "ClientAsteroidMiner.h"

namespace autotests { namespace client {

bool AsteroidMiner::getSpecification(AsteroidMinerSpecification &specification)
{
  spex::Message request;
  request.mutable_asteroid_miner()->set_specification_req(true);
  if (!send(request))
    return false;

  spex::IAsteroidMiner response;
  if (!wait(response))
    return false;
  if (response.choice_case() != spex::IAsteroidMiner::kSpecification)
    return false;

  specification.m_nMaxDistance   = response.specification().max_distance();
  specification.m_nCycleTimeMs   = response.specification().cycle_time_ms();
  specification.m_nYieldPerCycle = response.specification().yeild_pre_cycle();
  return true;
}

}}  // namespace aytitests::client
