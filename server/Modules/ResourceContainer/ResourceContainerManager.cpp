#include "ResourceContainerManager.h"
#include "ResourceContainer.h"

namespace modules {

using AllModules = utils::GlobalContainer<ResourceContainer>;

bool ResourceContainerManager::prepareAdditionalStage(
    [[maybe_unused]] uint16_t nStageId,
    [[maybe_unused]] uint32_t nIntervalUs,
    [[maybe_unused]] uint64_t nNowUs)
{
  m_nNextId.store(0);
  return !AllModules::empty();
}

void ResourceContainerManager::proceedAdditionalStage(
    [[maybe_unused]] uint16_t nStageId,
    [[maybe_unused]] uint32_t nIntervalUs,
    [[maybe_unused]] uint64_t nNowUs)
{
  uint32_t nId = m_nNextId.fetch_add(1);
  const size_t nTotalModules = AllModules::TotalInstancies();
  for (; nId < nTotalModules; nId = m_nNextId.fetch_add(1))
  {
    ResourceContainer* pModule = AllModules::Instance(nId);
    if (!pModule || !pModule->isOnline())
      continue;
    pModule->sendUpdatesIfRequired();
  }
}

} // namespace modules
