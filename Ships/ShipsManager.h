#pragma once

#include <atomic>
#include <mutex>
#include <vector>
#include <Conveyor/IAbstractLogic.h>
#include <Utils/Mutex.h>

namespace ships {

class Ship;
using ShipWeakPtr = std::weak_ptr<Ship>;

class ShipsManager : public conveyor::IAbstractLogic
{
public:
  ShipsManager();

  void addNewOne(ShipWeakPtr pCommandCenter);

  // overrides from IAbstractLogic interface
  uint16_t getStagesCount() { return 1; }
  bool prephareStage(uint16_t nStageId);
  void proceedStage(uint16_t nStageId, uint32_t nIntervalUs);

private:
  std::vector<ShipWeakPtr> m_Ships;

  size_t             m_nRemovingZombiesTimeout;
  std::atomic_size_t m_nNextId;
  utils::Mutex       m_Mutex;
};

using ShipsManagerPtr     = std::shared_ptr<ShipsManager>;
using ShipsManagerWeakPtr = std::weak_ptr<ShipsManager>;

} // namespace modules
