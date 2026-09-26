#pragma once

#include <stdint.h>

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientRCS.h>
#include "AbstractProcedure.h"
#include <Geometry/Point.h>

namespace autotests { namespace client {

class Navigation
{
public:
  Navigation(ShipPtr pShip)
    : m_pShip(pShip), m_pRCS(std::make_shared<RCS>())
  {}

  bool initialize();

  AbstractProcedurePtr MakeMoveToProcedure(geometry::Point const& target);

private:
  ShipPtr   m_pShip;
  RCSPtr m_pRCS;
};

}} // namespace autotests::client
