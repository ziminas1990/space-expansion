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
public:
  virtual ~BaseObjectFilter() = default;

  virtual void proceed() = 0;

  void     schedule(uint64_t nWhen) { m_nWhen = nWhen; }
  uint64_t when() const { return m_nWhen; }

  void prephare() { m_nNextId.store(0); }

protected:
  uint32_t yieldId() { return m_nNextId.fetch_add(1); }

private:
  uint64_t m_nWhen;
    // When filter should be proceeded
  std::atomic_uint32_t m_nNextId = 0;
};

using BaseObjectFilterPtr     = std::shared_ptr<BaseObjectFilter>;
using BaseObjectFilterWeakPtr = std::weak_ptr<BaseObjectFilter>;


template<typename ObjectType, typename FilterFunctor>
class TypedObjectFilter : public BaseObjectFilter
{
  using Container = utils::GlobalContainer<ObjectType>;
public:
  TypedObjectFilter(FilterFunctor&& fFilter, size_t nReserved = 256)
    : m_fFilter(std::move(fFilter))
  {
    m_filteredInstances.reserve(nReserved);
  }

  void proceed() override
  {
    std::array<uint32_t, 32> m_buffer;
    size_t                   nElementsInBuffer = 0;

    for (uint32_t nObjectId = yieldId();
         nObjectId < Container::TotalInstancies();
         nObjectId = yieldId()) {
      ObjectType const& obj = Container::Instance(nObjectId);
      if (!m_fFilter(obj)) {
        continue;
      }

      m_buffer[nElementsInBuffer++] = nObjectId;
      if (nElementsInBuffer == m_buffer.size()) {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_filteredInstances.insert(m_filteredInstances.end(),
                                   m_buffer.begin(), m_buffer.end());
        nElementsInBuffer = 0;
      }
    }

    if (nElementsInBuffer) {
      std::lock_guard<std::mutex> guard(m_mutex);
      m_filteredInstances.insert(m_filteredInstances.end(),
                                 m_buffer.begin(),
                                 m_buffer.begin() + nElementsInBuffer);
    }
  }

  std::vector<uint32_t> const& filtered() const { return m_filteredInstances; }

private:
  FilterFunctor         m_fFilter;
  std::mutex            m_mutex;
  std::vector<uint32_t> m_filteredInstances;
};


class ObjectsFilteringManager : public conveyor::IAbstractLogic
{
  enum Stages {
    eLockFilters = 0,
    eReset
  };

public:

  void registerFilter(BaseObjectFilterPtr& pFilter);

  // overrides from conveyor::IAbstractLogic
  uint16_t getStagesCount() override { return 1; }
  bool prephareStage(uint16_t nStageId) override;
  void proceed(uint16_t nStageId, uint32_t nIntervalUs, uint64_t now) override;
  size_t getCooldownTimeUs() const override { return 0; }

private:
  std::vector<BaseObjectFilterWeakPtr> m_filters;
  std::vector<BaseObjectFilterPtr>     m_lockedFilters;
};

using ObjectsFilteringManagerPtr = std::shared_ptr<ObjectsFilteringManager>;

} // namespace tools
