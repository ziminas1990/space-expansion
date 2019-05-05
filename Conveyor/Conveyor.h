#pragma once

#include "IAbstractLogic.h"
#include <vector>
#include <boost/fiber/barrier.hpp>

namespace conveyor
{

class Conveyor
{
public:
  Conveyor(uint16_t nTotalNumberOfThreads);

  void addLogicToChain(IAbstractLogicPtr pLogic);

  void proceed(size_t nIntervalUs);
  [[noreturn]] void joinAsSlave();

private:
  struct LogicContext
  {
    IAbstractLogicPtr m_pLogic;
    size_t            m_nDoNotDisturbUntil;
    size_t            m_nLastProceedAt;
  };

private:
  boost::fibers::barrier    m_Barrier;
  std::vector<LogicContext> m_LogicChain;

  struct State
  {
    size_t          nCurrentTimeUs  = 0;
    IAbstractLogic* pSelectedLogic  = nullptr;
    uint16_t        nStageId        = 0;
    size_t          nLastIntervalUs = 0;
  } m_State;
};

} // namespace conveoyr
