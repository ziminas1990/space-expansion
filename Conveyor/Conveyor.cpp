#include "Conveyor.h"

namespace conveyor
{

Conveyor::Conveyor(uint16_t nTotalNumberOfThreads)
  : m_Barrier(nTotalNumberOfThreads)
{}

void Conveyor::addLogicToChain(IAbstractLogicPtr&& pLogic)
{
  m_LogicChain.emplace_back(std::move(pLogic));
}

void Conveyor::proceed(size_t nTicksCount)
{
  m_State.reset();
  m_State.nTicksCount = nTicksCount;
  for(uint16_t nLogicId = 0; nLogicId < m_LogicChain.size(); ++nLogicId)
  {
    m_State.nLogicId = nLogicId;
    IAbstractLogicPtr& pLogic = m_LogicChain[nLogicId];
    uint16_t nTotalStages = pLogic->getStagesCount();
    for (uint16_t nStageId = 0; nStageId < nTotalStages; ++nStageId)
    {
      if (!pLogic->prephareStage(nStageId))
        continue;
      m_State.nStageId = nStageId;
      m_Barrier.wait();
      pLogic->proceedStage(nStageId, nTicksCount);
      m_Barrier.wait();
    }
  }
}

void Conveyor::joinAsSlave()
{
  while (true) {
    m_Barrier.wait();
    IAbstractLogicPtr& pLogic = m_LogicChain[m_State.nLogicId];
    pLogic->proceedStage(m_State.nStageId, m_State.nTicksCount);
    m_Barrier.wait();
  }
}

void Conveyor::State::reset()
{
  nLogicId    = 0;
  nStageId    = 0;
  nTicksCount = 0;
}

} // namespace conveoyr
