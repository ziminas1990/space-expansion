#include "Conveyor.h"

namespace conveyor
{

Conveyor::Conveyor(uint16_t nTotalNumberOfThreads)
  : m_Barrier(nTotalNumberOfThreads)
{}

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
  m_State.nCurrentTimeUs  += nIntervalUs;
  m_State.pSelectedLogic  = nullptr;
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
      if (!pLogic->prephareStage(nStageId))
        continue;
      m_State.nStageId = nStageId;
      m_Barrier.wait();
      pLogic->proceedStage(nStageId, nIntervalUs);
      m_Barrier.wait();
    }
    context.m_nDoNotDisturbUntil += pLogic->getCooldownTimeUs();
    context.m_nLastProceedAt      = m_State.nCurrentTimeUs;
  }
}

void Conveyor::joinAsSlave()
{
  while (true) {
    m_Barrier.wait();
    m_State.pSelectedLogic->proceedStage(m_State.nStageId, m_State.nLastIntervalUs);
    m_Barrier.wait();
  }
}

} // namespace conveoyr
