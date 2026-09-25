#pragma once

#include <memory>
#include <stdint.h>

#include <Autotests/ClientSDK/ClientBaseModule.h>

namespace autotests { namespace client {

struct HoverEngineSpecification
{
  uint32_t nMaxThrust;
};

class HoverEngine : public ClientBaseModule
{
public:
  bool getSpecification(HoverEngineSpecification& specification);

  // Sets the thrust magnitude, in newtons, for nDurationMs of ingame time.
  // The command has no reply.
  bool setThrust(uint32_t nThrust, uint32_t nDurationMs, uint64_t nWhenUs = 0);
  bool getThrust(uint32_t& nThrust);
  bool monitor(uint32_t& nThrust);
  bool waitThrust(uint32_t& nThrust, uint16_t nTimeout = 500);
};

using HoverEnginePtr = std::shared_ptr<HoverEngine>;

}}  // namespace autotests::client
