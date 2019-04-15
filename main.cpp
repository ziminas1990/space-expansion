#include <thread>
#include "Conveyor/Conveyor.h"
#include <atomic>

#define LOG_PREFIX "Logic #" << m_nLogicId << ", stage #" << nStageId << ": "
#define LOG(arg) std::cout << LOG_PREFIX << arg;

class TestLogic : public conveyor::IAbstractLogic
{
public:
  TestLogic(int nLogicId) : m_nLogicId(nLogicId) {}

  // IAbstractLogic interface
  uint16_t getStagesCount() override { return 3; }
  bool prephareStage(uint16_t nStageId) override
  {
    std::cout << std::endl;
    LOG("prepharing stage...");
    m_Value.store(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << "DONE!" << std::endl;
    return true;
  }
  void proceedStage(uint16_t, size_t) override
  {
    for(size_t i = 0; i < 3; ++i) {
      int nValue = m_Value.fetch_add(1);
      std::cout << "  Thread #" << std::this_thread::get_id() << ": " << nValue << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
  }

private:
  int m_nLogicId;
  std::atomic_int m_Value;
};

using TestLogicUptr = std::unique_ptr<TestLogic>;

int main(int, char*[])
{
  uint16_t nTotalThreadsCount = 4;
  conveyor::Conveyor conveyor(nTotalThreadsCount);

  for(size_t i = 0; i < 3; ++i)
    conveyor.addLogicToChain(std::make_unique<TestLogic>(i));

  for(size_t i = 1; i < nTotalThreadsCount; ++i)
    new std::thread([&conveyor]() { conveyor.joinAsSlave();} );

  conveyor.proceed(1);
  return 0;
}
