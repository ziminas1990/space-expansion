#pragma once

#include <Autotests/ClientSDK/Interfaces.h>
#include <Autotests/ClientSDK/ClientBaseModule.h>
#include <World/Resources.h>
#include <stdint.h>

namespace autotests { namespace client {

struct AsteroidMinerSpecification
{
  uint32_t m_nMaxDistance;
  uint32_t m_nCycleTimeMs;
  uint32_t m_nYieldPerCycle;
};

class AsteroidMiner : public ClientBaseModule
{
public:
  enum Status {
    eSuccess,
    eTimeout,
    eTransportError,
    eUnexpectedMessage,
    eServerError,
    eMinerIsBusy,
    eMinerIsIdle,
    eAsteroidDoesntExist,
    eAsteroidTooFar,
    eNoSpaceAvaliable
  };

  bool getSpecification(AsteroidMinerSpecification& specification);

  Status startMining(uint32_t nAsteroidId, world::Resources::Type eResourceType);
  Status stopMining();

  bool waitMiningReport(double& nAmount, uint16_t nTimeout = 500);
  bool waitError(Status eExpectedStatus);

};

using AsteroidMinerPtr = std::shared_ptr<AsteroidMiner>;

}}  // namespace autotests::client
