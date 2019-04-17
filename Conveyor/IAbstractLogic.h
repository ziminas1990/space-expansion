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
  virtual void proceedStage(uint16_t nStageId, size_t nTicksCount) = 0;
};

using IAbstractLogicUptr = std::unique_ptr<IAbstractLogic>;

} // namespace conveyor
