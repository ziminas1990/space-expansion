#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <memory>

namespace conveyor {

// Functions getStagesCount() and prephareStage() will be called in main thread and they
// don't need to be thread safe.
// Function proceedStage() could be called in several threads and must be thread safe.
class IAbstractLogic
{
public:
  virtual ~IAbstractLogic() = default;

  // Return a number of total stages. A 'prephare()' method for each stage
  // will be called then.
  virtual uint16_t getStagesCount() = 0;

  // This function will be called for each stage before calling proceedStage().
  // This function always run in one thread, so you do NOT need any synchronization
  // in it's implementation. If function returns false, than stage will be skipped
  virtual bool prephare(uint16_t nStageId, uint32_t nIntervalUs, uint64_t now) = 0;

  // This function will be called for every stage, if prhephareStage() returned true
  // This function will be called from a number of threads, so you must provide thread
  // synchronization if it is required.
  // nStageId    - number of stage;
  // nIntervalUs - in-game time, that passed since last proceed (in microseconds);
  // now         - microseconds since tie world has been started;
  virtual void proceed(uint16_t nStageId, uint32_t nIntervalUs, uint64_t now) = 0;

  // This function returns a number of microseconds (of in-game time), during wich
  // logic shouldn't be proceeded again
  // If functions returns 1 or 0, it means "procced again as soon as possible"
  // NOTE: By default, logic won't be proceeded more than 100 times per second
  // NOTE: in-game time can differ from real time
  virtual size_t getCooldownTimeUs() const { return 10 * 1000; }
};

using IAbstractLogicPtr = std::shared_ptr<IAbstractLogic>;

} // namespace conveyor
