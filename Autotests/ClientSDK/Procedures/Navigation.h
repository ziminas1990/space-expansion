#pragma once

#include <stdint.h>

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientEngine.h>
#include "AbstractProcedure.h"
#include <Geometry/Point.h>

namespace autotests { namespace client {

class Navigation : public IProceedable
{
public:
  Navigation(ShipPtr pShip)
    : m_pShip(pShip), m_pEngine(std::make_shared<Engine>())
  {}

  bool initialize();

  // overrides from IProceedable
  void proceed(uint32_t nDeltaUs) override;
  bool isComplete() const override;
  bool isSucceed() const { return isComplete() && m_pProcedure->isSucceed(); }

  void moveTo(geometry::Point const& position);

  void interrupt() {
    if (m_pProcedure)
      m_pProcedure->interrupt();
  }

private:
  ShipPtr   m_pShip;
  EnginePtr m_pEngine;
  AbstractProcedurePtr m_pProcedure;
};

}} // namespace autotests::client
