#pragma once

#include <atomic>
#include <mutex>
#include <vector>
#include <Conveyor/IAbstractLogic.h>

namespace modules {

class CommandCenter;
using CommandCenterWeakPtr = std::weak_ptr<CommandCenter>;

class CommandCenterManager : public conveyor::IAbstractLogic
{
public:
  CommandCenterManager();

  void addNewOne(CommandCenterWeakPtr pCommandCenter);

  // overrides from IAbstractLogic interface
  uint16_t getStagesCount() { return 1; }
  bool prephareStage(uint16_t nStageId);
  void proceedStage(uint16_t nStageId, size_t nIntervalUs);

private:
  std::vector<CommandCenterWeakPtr> m_CommandCentres;

  size_t             m_nRemovingZombiesTimeout;
  std::atomic_size_t m_nNextId;
  std::mutex         m_Mutex;
};

using CommandCenterManagerPtr     = std::shared_ptr<CommandCenterManager>;
using CommandCenterManagerWeakPtr = std::weak_ptr<CommandCenterManager>;

} // namespace modules
