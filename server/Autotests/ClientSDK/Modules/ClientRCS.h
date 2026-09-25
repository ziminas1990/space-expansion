#pragma once

#include <Autotests/ClientSDK/ClientBaseModule.h>
#include <Geometry/Vector.h>
#include <stdint.h>
#include <vector>

namespace autotests { namespace client {

struct RCSSpecification
{
  uint32_t nMaxThrust;
};

class RCS : public ClientBaseModule
{
public:
  bool getSpecification(RCSSpecification& specification);

  bool setThrust(geometry::Vector thrust, uint32_t nDurationMs,
                 uint64_t nWhenUs = 0);
  bool getThrust(geometry::Vector& thrust);
  bool monitor(geometry::Vector& thrust);
  bool waitThrust(geometry::Vector& thrust, uint16_t nTimeout = 500);

};

using RCSPtr = std::shared_ptr<RCS>;

}}  // namespace autotests::client
