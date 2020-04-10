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
  ~Conveyor();

  void addLogicToChain(IAbstractLogicPtr pLogic);

  void proceed(uint32_t nIntervalUs);
  void joinAsSlave();

  void stop();

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
  uint64_t                  m_now         = 0;

  struct State
  {
    size_t          nCurrentTimeUs  = 0;
    IAbstractLogic* pSelectedLogic  = nullptr;
    uint16_t        nStageId        = 0;
    size_t          nLastIntervalUs = 0;
  } m_State;
};

} // namespace conveoyr
