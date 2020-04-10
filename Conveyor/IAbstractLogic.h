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

  virtual uint16_t getStagesCount() = 0;

  // This function will be called for each stage before calling proceedStage().
  // If function returns false, than stage will be skipped
  virtual bool prephareStage(uint16_t nStageId) = 0;

  // This function will be called for every stage, if prhephareStage() returned true
  // nIntervalUs - in-game time (im microseconds), that passed since last proceed
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
