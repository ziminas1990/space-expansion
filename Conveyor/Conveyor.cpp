#include "Conveyor.h"

#include <thread>

namespace conveyor
{

thread_local static bool gContinueSlave = true;

//========================================================================================
// SlaveTerminator
//========================================================================================

// Helper logic, that will be used to terminate all attached slave threads
class SlaveTerminator : public IAbstractLogic
{
public:
  // overrides from IAbstractLogic interface
  uint16_t getStagesCount()        override { return 1; }
  bool     prephare(uint16_t, uint32_t, uint64_t) override { return true; }
  void     proceed(uint16_t, uint32_t, uint64_t) override { gContinueSlave = false; }
};

//========================================================================================
// Conveyor
//========================================================================================

Conveyor::Conveyor(uint16_t nTotalNumberOfThreads)
  : m_Barrier(nTotalNumberOfThreads)
{}

Conveyor::~Conveyor()
{
  stop();
}

void Conveyor::addLogicToChain(IAbstractLogicPtr pLogic)
{
  LogicContext context;
  context.m_pLogic             = pLogic;
  context.m_nLastProceedAt     = m_State.nCurrentTimeUs;
  context.m_nDoNotDisturbUntil = 0;
  m_LogicChain.push_back(std::move(context));
}

void Conveyor::proceed(uint32_t nIntervalUs)
{
  m_now                  += nIntervalUs;
  m_State.nCurrentTimeUs += nIntervalUs;
  m_State.pSelectedLogic = nullptr;
  for(LogicContext& context : m_LogicChain)
  {
    if (m_State.nCurrentTimeUs < context.m_nDoNotDisturbUntil)
      continue;

    // prepharing state for proceeding selected logic
    IAbstractLogic* pLogic  = context.m_pLogic.get();
    m_State.pSelectedLogic  = pLogic;
    m_State.nLastIntervalUs = m_State.nCurrentTimeUs - context.m_nLastProceedAt;

    // proceeding logic stages
    uint16_t nTotalStages = pLogic->getStagesCount();
    for (uint16_t nStageId = 0; nStageId < nTotalStages; ++nStageId)
    {
      if (!pLogic->prephare(nStageId, nIntervalUs, m_now))
        continue;
      m_State.nStageId = nStageId;
      m_Barrier.wait();
      pLogic->proceed(nStageId, static_cast<uint32_t>(m_State.nLastIntervalUs), m_now);
      m_Barrier.wait();
    }
    context.m_nDoNotDisturbUntil += pLogic->getCooldownTimeUs();
    context.m_nLastProceedAt      = m_State.nCurrentTimeUs;
  }
}

void Conveyor::joinAsSlave()
{
  gContinueSlave = true;
  while (gContinueSlave) {
    m_Barrier.wait();
    m_State.pSelectedLogic->proceed(
          m_State.nStageId, static_cast<uint32_t>(m_State.nLastIntervalUs), m_now);
    m_Barrier.wait();
  }
}

void Conveyor::stop()
{
  SlaveTerminator terminator;
  m_State.pSelectedLogic = &terminator;
  m_Barrier.wait();
  // all slave threads will set gContinueSlave to false and after next barrier
  // they will quit from Conveyor::joinAsSlave() function
  m_Barrier.wait();
  std::this_thread::yield();  // for sure
}

} // namespace conveoyr
