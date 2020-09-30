#include "ClientAsteroidMiner.h"
#include <Utils/ItemsConverter.h>

namespace autotests { namespace client {

static AsteroidMiner::Status convert(spex::IAsteroidMiner::Status eStatus) {
  switch (eStatus) {
    case spex::IAsteroidMiner::SUCCESS:
      return AsteroidMiner::eSuccess;
    case spex::IAsteroidMiner::INTERNAL_ERROR:
      return AsteroidMiner::eServerError;
    case spex::IAsteroidMiner::ASTEROID_DOESNT_EXIST:
      return AsteroidMiner::eAsteroidDoesntExist;
    case spex::IAsteroidMiner::MINER_IS_BUSY:
      return AsteroidMiner::eMinerIsBusy;
    case spex::IAsteroidMiner::ASTEROID_TOO_FAR:
      return AsteroidMiner::eAsteroidTooFar;
    case spex::IAsteroidMiner::MINER_IS_IDLE:
      return AsteroidMiner::eMinerIsIdle;
    case spex::IAsteroidMiner::NO_SPACE_AVAILABLE:
      return AsteroidMiner::eNoSpaceAvailable;
    case spex::IAsteroidMiner::NOT_BOUND_TO_CARGO:
      return AsteroidMiner::eNotBoundToCargo;
    case spex::IAsteroidMiner::INTERRUPTED_BY_USER:
      return AsteroidMiner::eInterruptedByUser;
    default:
      assert(nullptr == "Unexpected status");
  }
}

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
  specification.m_nYieldPerCycle = response.specification().yield_per_cycle();
  return true;
}

AsteroidMiner::Status AsteroidMiner::bindToCargo(std::string const& sCargoName)
{
  spex::Message request;
  request.mutable_asteroid_miner()->set_bind_to_cargo(sCargoName);
  if (!send(request))
    return eTransportError;

  spex::IAsteroidMiner response;
  if (!wait(response))
    return eTimeout;
  if (response.choice_case() != spex::IAsteroidMiner::kBindToCargoStatus)
    return eUnexpectedMessage;

  return convert(response.bind_to_cargo_status());
}

AsteroidMiner::Status AsteroidMiner::startMining(
    uint32_t nAsteroidId, world::Resource::Type eResourceType)
{
  spex::Message message;
  spex::IAsteroidMiner* request = message.mutable_asteroid_miner();
  spex::IAsteroidMiner::MiningTask* body = request->mutable_start_mining();

  body->set_asteroid_id(nAsteroidId);
  body->set_resource(utils::convert(eResourceType));
  if (!send(message))
    return eTransportError;

  spex::IAsteroidMiner response;
  if (!wait(response))
    return eTimeout;
  if (response.choice_case() != spex::IAsteroidMiner::kStartMiningStatus)
    return eUnexpectedMessage;

  return convert(response.start_mining_status());
}

AsteroidMiner::Status AsteroidMiner::stopMining()
{
  spex::Message message;
  message.mutable_asteroid_miner()->set_stop_mining(true);
  if (!send(message))
    return eTransportError;

  spex::IAsteroidMiner response;
  if (!wait(response))
    return eTimeout;
  if (response.choice_case() != spex::IAsteroidMiner::kStopMiningStatus)
    return eUnexpectedMessage;

  Status eStopStatus = convert(response.stop_mining_status());
  if (eStopStatus != Status::eSuccess)
    return eStopStatus;

  return waitMiningIsStoppedInd(eStopStatus) ? Status::eSuccess : Status::eTimeout;
}

bool AsteroidMiner::waitMiningReport(double& nAmount, uint16_t nTimeout)
{
  spex::IAsteroidMiner response;
  if (!wait(response, nTimeout))
    return false;
  if (response.choice_case() != spex::IAsteroidMiner::kMiningReport)
    return false;

  nAmount = response.mining_report().amount();
  return true;
}

bool AsteroidMiner::waitMiningIsStoppedInd(Status& eStatus, uint16_t nTimeout)
{
  spex::IAsteroidMiner response;
  if (!wait(response, nTimeout))
    return false;
  if (response.choice_case() != spex::IAsteroidMiner::kMiningIsStopped)
    return false;
  eStatus = convert(response.mining_is_stopped());
  return true;
}

}}  // namespace aytitests::client
