#include "ObjectsFilter.h"

namespace tools {

void ObjectsFilteringManager::registerFilter(BaseObjectFilterPtr const& pFilter)
{
  m_filters.push_back(pFilter);
}

bool ObjectsFilteringManager::prephare(uint16_t nStageId, uint32_t, uint64_t now)
{
  assert(nStageId == 0);

  m_lockedFilters.clear();
  for(size_t i = 0; i < m_filters.size(); ++i) {
    BaseObjectFilterPtr pLockedFilter = m_filters[i].lock();
    if (pLockedFilter) {
      if (pLockedFilter->isTimeToUpdate(now)) {
        pLockedFilter->prephare();
        m_lockedFilters.push_back(std::move(pLockedFilter));
      }
    } else {
      // To remove element, just swap it with last element and than remove last element
      if (i < m_filters.size() - 1)
        std::swap(m_filters.at(i), m_filters.back());
      m_filters.pop_back();
    }
  }
  return !m_lockedFilters.empty();
}

void ObjectsFilteringManager::proceed(uint16_t, uint32_t, uint64_t)
{
  for (BaseObjectFilterPtr pFilter : m_lockedFilters) {
    pFilter->proceed();
  }
}

void BaseObjectFilter::updateAsSoonAsPossible()
{
  m_eUpdatePolicy = eUpdateASAP;
}

void BaseObjectFilter::updateAt(uint64_t nWhenUs)
{
  m_eUpdatePolicy = eUpdateSceduled;
  m_nUpdateAt     = nWhenUs;
}

bool BaseObjectFilter::isTimeToUpdate(uint64_t now) const
{
  switch (m_eUpdatePolicy) {
    case eUpdateNever:
      return false;
    case eUpdateASAP:
      return true;
    case eUpdateSceduled:
      return m_nUpdateAt <= now;
  }
  return false;
}

void BaseObjectFilter::prephare()
{
  m_nNextId.store(0);
  m_nUpdateAt = eUpdateNever;
}

} // namespace tools
