#pragma once

#include <stdint.h>

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientEngine.h>
#include "AbstractProcedure.h"
#include <Geometry/Point.h>

namespace autotests { namespace client {

class Navigation
{
public:
  Navigation(ShipPtr pShip)
    : m_pShip(pShip), m_pEngine(std::make_shared<Engine>())
  {}

  bool initialize();

  AbstractProcedurePtr MakeMoveToProcedure(
      geometry::Point const& target, uint32_t nSyncIntervalMs);

private:
  ShipPtr   m_pShip;
  EnginePtr m_pEngine;
};

}} // namespace autotests::client
