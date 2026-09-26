#include "Navigation.h"

#include <cmath>

#include <Autotests/TestUtils/FlightPlanner.h>
#include "FindModule.h"

namespace autotests { namespace client {

//========================================================================================
// MovingProcedure
//========================================================================================

// Moves the ship to the given position in five steps:
//   1. stop, so the following plan can assume the ship is at rest
//   2. from the measured position, plan the flight
//   3. fly that plan
//   4. from the measured position and velocity, plan a final stop
//   5. fly that stop
// RCS aims the force directly, so planned turns are dropped. A very high
// turn rate makes each of them last about a millisecond.
class MovingProcedure : public AbstractProcedure
{
public:
  MovingProcedure(ShipPtr pShip, RCSPtr pRCS, geometry::Point target)
    : m_pShip(pShip), m_pRCS(pRCS), m_target(target)
  {}

  void proceed(uint32_t nDeltaUs) override;

protected:
  bool prephare() override;

private:
  enum class State {
    eInitialStop,
    ePlanFlight,
    eFlight,
    ePlanFinalStop,
    eFinalStop,
  };

  void proceedInitialStop(uint32_t nDeltaUs);
  void proceedPlanFlight();
  void proceedFlight(uint32_t nDeltaUs);
  void proceedPlanFinalStop();
  void proceedFinalStop(uint32_t nDeltaUs);

  void beginSequence(autotests::FlightPlan const& plan);
  // True while the current sequence still has a burn to run or to wait out.
  bool runSequence(uint32_t nDeltaUs);
  bool startNextBurn();

  ShipPtr         m_pShip;
  RCSPtr          m_pRCS;
  geometry::Point m_target;

  autotests::ShipInfo m_shipInfo {};
  State m_state = State::eInitialStop;

  std::vector<autotests::HoverEngineBurn> m_burns;
  size_t   m_nNextBurn        = 0;
  uint64_t m_nUsUntilNextBurn = 0;
};

bool MovingProcedure::prephare()
{
  geometry::Point  position;
  geometry::Vector velocity;
  if (!m_pShip->getPosition(position, velocity)) {
    return false;
  }

  ShipState shipState;
  if (!m_pShip->getState(shipState)) {
    return false;
  }
  RCSSpecification engineSpec;
  if (!m_pRCS->getSpecification(engineSpec)) {
    return false;
  }

  constexpr double kInstantTurnRadPerSec = 1000.0 * M_PI;
  m_shipInfo = autotests::ShipInfo {
    shipState.nWeight,
    static_cast<double>(engineSpec.nMaxThrust),
    kInstantTurnRadPerSec
  };

  m_state = State::eInitialStop;
  beginSequence(autotests::FlightPlanner::stop(m_shipInfo, velocity));
  if (m_burns.empty()) {
    return true;
  }
  return startNextBurn();
}

void MovingProcedure::proceed(uint32_t nDeltaUs)
{
  switch (m_state) {
    case State::eInitialStop:   proceedInitialStop(nDeltaUs); break;
    case State::ePlanFlight:    proceedPlanFlight();          break;
    case State::eFlight:        proceedFlight(nDeltaUs);      break;
    case State::ePlanFinalStop: proceedPlanFinalStop();       break;
    case State::eFinalStop:     proceedFinalStop(nDeltaUs);   break;
  }
}

void MovingProcedure::proceedInitialStop(uint32_t nDeltaUs)
{
  if (runSequence(nDeltaUs)) {
    return;
  }
  m_state = State::ePlanFlight;
  proceed(0);
}

void MovingProcedure::proceedPlanFlight()
{
  geometry::Point position;
  if (!m_pShip->getPosition(position)) {
    failed();
    return;
  }

  beginSequence(autotests::FlightPlanner::plan(m_shipInfo, position, m_target));
  m_state = State::eFlight;
  proceed(0);
}

void MovingProcedure::proceedFlight(uint32_t nDeltaUs)
{
  if (runSequence(nDeltaUs)) {
    return;
  }
  m_state = State::ePlanFinalStop;
  proceed(0);
}

void MovingProcedure::proceedPlanFinalStop()
{
  geometry::Point  position;
  geometry::Vector velocity;
  if (!m_pShip->getPosition(position, velocity)) {
    failed();
    return;
  }

  // One second is long enough that a small residual speed becomes a gentle
  // burn instead of a one-tick full-thrust pulse.
  constexpr double kMinStopBurnMs = 1000.0;
  beginSequence(
      autotests::FlightPlanner::stop(m_shipInfo, velocity, kMinStopBurnMs));
  m_state = State::eFinalStop;
  proceed(0);
}

void MovingProcedure::proceedFinalStop(uint32_t nDeltaUs)
{
  if (runSequence(nDeltaUs)) {
    return;
  }
  finished();
}

void MovingProcedure::beginSequence(autotests::FlightPlan const& plan)
{
  m_burns.clear();
  m_nNextBurn = 0;
  m_nUsUntilNextBurn = 0;
  for (autotests::Maneuver const& maneuver : plan.maneuvers) {
    if (auto const* pBurn = std::get_if<autotests::HoverEngineBurn>(&maneuver)) {
      if (pBurn->nDurationMs > 0) {
        m_burns.push_back(*pBurn);
      }
    }
  }
}

bool MovingProcedure::runSequence(uint32_t nDeltaUs)
{
  if (m_nUsUntilNextBurn > nDeltaUs) {
    m_nUsUntilNextBurn -= nDeltaUs;
    return true;
  }
  m_nUsUntilNextBurn = 0;

  if (m_nNextBurn >= m_burns.size()) {
    return false;
  }
  if (!startNextBurn()) {
    failed();
  }
  return true;
}

bool MovingProcedure::startNextBurn()
{
  autotests::HoverEngineBurn const& burn = m_burns[m_nNextBurn];
  ++m_nNextBurn;
  m_nUsUntilNextBurn = static_cast<uint64_t>(burn.nDurationMs) * 1000;

  // RCS thrust is always the module maximum. A weaker burn from the plan
  // keeps the same impulse as a shorter full-thrust burn.
  if (!(m_shipInfo.nMaxThrust > 0) || burn.nThrust == 0) {
    return true;
  }
  double nFraction = std::clamp(
    static_cast<double>(burn.nThrust) / m_shipInfo.nMaxThrust, 0.0, 1.0);
  const uint32_t nThrustMs = static_cast<uint32_t>(std::llround(nFraction * burn.nDurationMs));
  return m_pRCS->setThrust(burn.expectedDirection, nThrustMs);
}

//========================================================================================
// Navigation
//========================================================================================

bool Navigation::initialize()
{
  return FindMostPowerfulRCS(*m_pShip, *m_pRCS);
}

AbstractProcedurePtr Navigation::MakeMoveToProcedure(geometry::Point const& target)
{
  if (!m_pShip || !m_pRCS)
    return nullptr;
  if (!m_pShip->isAttached() || ! m_pRCS->isAttached())
    return nullptr;
  return std::make_shared<MovingProcedure>(m_pShip, m_pRCS, target);
}

}}  // namespace autotests::Client
