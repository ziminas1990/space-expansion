#pragma once

#include <memory>
#include <vector>
#include <atomic>
#include "PhysicalObject.h"
#include <Utils/Mutex.h>
#include <Conveyor/IAbstractLogic.h>

namespace newton {

class NewtonEngine : public conveyor::IAbstractLogic
{
public:
  // overrides from IAbstractLogic interface
  uint16_t getStagesCount() override { return 1; }
  bool prephareStage(uint16_t nStageId) override;
  void proceed(uint16_t nStageId, uint32_t nIntervalUs, uint64_t now) override;
  size_t getCooldownTimeUs() const override { return 0; }

private:
  utils::Mutex m_Mutex;

  std::atomic_size_t m_nNextObjectId;
};

using NewtonEnginePtr = std::shared_ptr<NewtonEngine>;

} // namespace newton
