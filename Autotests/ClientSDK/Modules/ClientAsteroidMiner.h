#pragma once

#include <Autotests/ClientSDK/Interfaces.h>
#include <Autotests/ClientSDK/ClientBaseModule.h>
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
  bool getSpecification(AsteroidMinerSpecification& specification);

};

using AsteroidMinerPtr = std::shared_ptr<AsteroidMiner>;

}}  // namespace autotests::client
