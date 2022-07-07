#pragma once

#include <Conveyor/IAbstractLogic.h>

namespace autotests {

class FreezableLogic : public virtual conveyor::IAbstractLogic
{
private:
  conveyor::IAbstractLogicPtr m_pLogic;
  bool                        m_lFreezed;

public:

  FreezableLogic(conveyor::IAbstractLogicPtr pLogic, bool lFreezed = false)
  : m_pLogic(pLogic), m_lFreezed(lFreezed)
  {}

  void freeze() { m_lFreezed = true; }
  void resume() { m_lFreezed = false; }
  bool freezed() const { return m_lFreezed; }

  uint16_t getStagesCount() override { return m_pLogic->getStagesCount(); }

  bool prephare(uint16_t nStageId, uint32_t nIntervalUs, uint64_t now) override
  {
    return !m_lFreezed && m_pLogic->prephare(nStageId, nIntervalUs, now);
  };

  void proceed(uint16_t nStageId, uint32_t nIntervalUs, uint64_t now) override {
    m_pLogic->proceed(nStageId, nIntervalUs, now);
  }

  size_t getCooldownTimeUs() const override { 
    return m_pLogic->getCooldownTimeUs();
  }
};

using FreezableLogicPtr = std::shared_ptr<FreezableLogic>;

} // namespace autotests