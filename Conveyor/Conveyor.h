#include "IAbstractLogic.h"
#include <vector>
#include <boost/fiber/barrier.hpp>

namespace conveyor
{

class Conveyor
{
public:
  Conveyor(uint16_t nTotalNumberOfThreads);

  void addLogicToChain(IAbstractLogicUptr&& pLogic);

  void proceed(size_t nTicksCount);
  void joinAsSlave();

private:
  boost::fibers::barrier m_Barrier;
  std::vector<IAbstractLogicUptr> m_LogicChain;

  struct State
  {
    void reset();

    uint16_t nLogicId    = 0;
    uint16_t nStageId    = 0;
    size_t   nTicksCount = 0;
  } m_State;
};

} // namespace conveoyr
