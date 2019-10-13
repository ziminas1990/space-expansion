#pragma once

#include <Autotests/ClientSDK/Interfaces.h>
#include <Autotests/ClientSDK/ClientBaseModule.h>
#include <Geometry/Vector.h>
#include <stdint.h>
#include <vector>

namespace autotests { namespace client {

struct EngineSpecification
{
  uint32_t nMaxThrust;
};

class Engine : public ClientBaseModule
{
public:
  bool getSpecification(EngineSpecification& specification);

  bool setThrust(geometry::Vector thrust, uint32_t nDurationMs);
  bool getThrust(geometry::Vector& thrust);

};

using EnginePtr = std::shared_ptr<Engine>;

}}  // namespace autotests::client
