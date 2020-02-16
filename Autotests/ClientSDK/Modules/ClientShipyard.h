#pragma once

#include <Autotests/ClientSDK/ClientBaseModule.h>

namespace autotests { namespace client {

struct ShipyardSpecification {
  double m_nLaborPerSec;
};

class Shipyard : public ClientBaseModule
{
public:

  bool getSpecification(ShipyardSpecification& spec);

};

}}  // namespace autotests::client
