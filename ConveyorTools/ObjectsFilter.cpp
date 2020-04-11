#include "ObjectsFilter.h"

namespace tools {

void ObjectsFilteringManager::registerFilter(BaseObjectFilterPtr &pFilter)
{
  m_filters.push_back(pFilter);
}

bool ObjectsFilteringManager::prephare(uint16_t nStageId, uint32_t, uint64_t)
{
  assert(nStageId == 0);

  m_lockedFilters.clear();
  for(size_t i = 0; i < m_filters.size(); ++i) {
    BaseObjectFilterPtr pLockedFilter = m_filters[i].lock();
    if (pLockedFilter) {
      pLockedFilter->prephare();
      m_lockedFilters.push_back(std::move(pLockedFilter));
    } else {
      // To remove element, just swap it with last element and than remove last element
      if (i < m_filters.size() - 1)
        std::swap(m_filters.at(i), m_filters.back());
      m_filters.pop_back();
    }
  }
  return !m_lockedFilters.empty();
}

void ObjectsFilteringManager::proceed(uint16_t, uint32_t nIntervalUs, uint64_t now)
{
  for (BaseObjectFilterPtr pFilter : m_lockedFilters) {
    if (pFilter->when() <= now + nIntervalUs) {
      pFilter->proceed();
    }
  }
}

} // namespace tools
