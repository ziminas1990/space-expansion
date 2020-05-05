#include "Navigation.h"

#include <math.h>
#include "FindModule.h"

namespace autotests { namespace client {

//========================================================================================
// MovingProcedure
//========================================================================================

// This procedure moves ship to specified position
class MovingProcedure : public AbstractProcedure
{
public:
  MovingProcedure(ShipPtr pShip, EnginePtr pEngine, geometry::Point target,
                  uint32_t nSyncIntervalUs = 100000)
    : m_pShip(pShip), m_pEngine(pEngine), m_target(target),
      m_nSyncIntervalUs(nSyncIntervalUs), m_nTimeSinceLastSyncUs(nSyncIntervalUs),
      m_nTimeToUpdateUs(0)
  {}

  void proceed(uint32_t nDeltaUs) override;

private:
  ShipPtr   m_pShip;
  EnginePtr m_pEngine;

  geometry::Point m_target;

  uint32_t m_nSyncIntervalUs;
  uint32_t m_nTimeSinceLastSyncUs;

  uint32_t            m_nTimeToUpdateUs;
  EngineSpecification m_engineSpec;
  ShipState           m_shipState;
};

using MovingProcedurePtr = std::shared_ptr<MovingProcedure>;

void MovingProcedure::proceed(uint32_t nDeltaUs)
{
  m_nTimeSinceLastSyncUs += nDeltaUs;
  if (m_nTimeSinceLastSyncUs < m_nSyncIntervalUs)
    return;
  m_nTimeSinceLastSyncUs = 0;

  geometry::Point  position;
  geometry::Vector velocity;

  if (!m_pShip->getPosition(position, velocity)) {
    failed();
    return;
  }

  double distance = position.distance(m_target);
  if (distance < 1 && velocity.getLength() < 0.5) {
    // 1 meter and 1 m/s is OK
    finished();
    return;
  }

  if (m_nTimeToUpdateUs < nDeltaUs) {
    if (!m_pShip->getState(m_shipState)) {
      failed();
      return;
    }
    if (!m_pEngine->getSpecification(m_engineSpec)) {
      failed();
      return;
    }
    m_nTimeToUpdateUs = m_nSyncIntervalUs * 10;
  } else {
    m_nTimeToUpdateUs -= nDeltaUs;
  }


  double bestSpeed = sqrt(2 * distance * m_engineSpec.nMaxThrust / m_shipState.nWeight);
  geometry::Vector bestVelocity = position.vectorTo(m_target);
  bestVelocity.setLength(bestSpeed);

  double           nSyncIntervalSec = m_nSyncIntervalUs / 1000000.0;
  geometry::Vector bestAcceleration = (bestVelocity - velocity) / nSyncIntervalSec;
  geometry::Vector bestThrust       = bestAcceleration * m_shipState.nWeight;

  if (bestThrust.getLength() > m_engineSpec.nMaxThrust)
    bestThrust.setLength(m_engineSpec.nMaxThrust);
  if (!m_pEngine->setThrust(bestThrust, m_nSyncIntervalUs / 1000))
    failed();
}


//========================================================================================
// Navigation
//========================================================================================

bool Navigation::initialize()
{
  return FindMostPowerfulEngine(*m_pShip, *m_pEngine);
}

AbstractProcedurePtr Navigation::MakeMoveToProcedure(
    geometry::Point const& target, uint32_t nSyncIntervalMs)
{
  if (!m_pShip || !m_pEngine)
    return nullptr;
  if (!m_pShip->isAttached() || ! m_pEngine->isAttached())
    return nullptr;
  return std::make_shared<MovingProcedure>(
        m_pShip, m_pEngine, target, nSyncIntervalMs * 1000);
}

}}  // namespace autotests::Client
