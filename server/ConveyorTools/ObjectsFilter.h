#pragma once

#include <memory>
#include <vector>
#include <atomic>
#include <array>

#include <Conveyor/IAbstractLogic.h>
#include <Utils/GlobalContainer.h>

namespace tools {


class BaseObjectFilter
{
  enum UpdatePolicy {
    eUpdateNever,
    eUpdateASAP,
    eUpdateSceduled
  };

public:
  virtual ~BaseObjectFilter() = default;

  virtual void reset() = 0;
    // Reset all previously accumulated results. Will be always called in a single
    // thread before the 'proceed()' call. Doesn't need to be thread safe

  virtual void proceed() = 0;
    // This function may be called from different threads simultaneously, so it must
    // be thread safe. Use the 'yieldId()' call to get id of the next object to apply
    // filter logic on it.

  void updateAsSoonAsPossible();
  void updateAt(uint64_t nWhenUs);
  bool isTimeToUpdate(uint64_t now) const;
  bool isWaitingForUpdate() const { return m_eUpdatePolicy != eUpdateNever; }
    // Return true if update is requested but has not been done yet

  void prephare();
    // Reset filter and prephare it to be proceeded. It also reset update policy
    // to eUpdateNever, so filter won't be proceeded again until it is required

protected:
  uint32_t yieldId() { return m_nNextId.fetch_add(1); }

private:
  UpdatePolicy m_eUpdatePolicy = eUpdateNever;
  uint64_t     m_nUpdateAt     = 0;
    // When filter should be updated (if policy is eUpdateSceduled)
  std::atomic_uint32_t m_nNextId = 0;
    // Will be used by different threads from 'proceed()' call to get id of object
    // that should be checked
};

using BaseObjectFilterPtr     = std::shared_ptr<BaseObjectFilter>;
using BaseObjectFilterWeakPtr = std::weak_ptr<BaseObjectFilter>;


// Class to store filters and proceed them when is is required.
// Filters can be procceded in turns by deveral threads simultaneously and each thread may
// switch to next filter whenever he want, so there is no any synchronization between
// filters.
class ObjectsFilteringManager : public conveyor::IAbstractLogic
{
  enum Stages {
    eLockFilters = 0,
    eReset
  };

public:

  void registerFilter(BaseObjectFilterPtr const& pFilter);

  // overrides from conveyor::IAbstractLogic
  uint16_t getStagesCount() override { return 1; }
  bool prephare(uint16_t nStageId, uint32_t nIntervalUs, uint64_t now) override;
  void proceed(uint16_t nStageId, uint32_t nIntervalUs, uint64_t now) override;
  size_t getCooldownTimeUs() const override { return 0; }

private:
  std::vector<BaseObjectFilterWeakPtr> m_filters;
  std::vector<BaseObjectFilterPtr>     m_lockedFilters;
};

using ObjectsFilteringManagerPtr = std::shared_ptr<ObjectsFilteringManager>;

} // namespace tools
