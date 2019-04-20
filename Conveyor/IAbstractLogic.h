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
  virtual void proceedStage(uint16_t nStageId, size_t nIntervalUs) = 0;

  // This function returns a number of microseconds (of in-game time), during wich
  // logic shouldn't be proceeded again
  // If functions returns 1 or 0, it means "procced again as soon as possible"
  // Note: in-game time could differ from real time
  virtual size_t getCooldownTimeUs() const { return 1; }
};

using IAbstractLogicPtr = std::shared_ptr<IAbstractLogic>;

} // namespace conveyor
