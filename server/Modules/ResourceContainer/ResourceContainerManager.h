#include <Modules/CommonModulesManager.h>
#include "ResourceContainer.h"

namespace modules {


class ResourceContainerManager
    : public CommonModulesManager<ResourceContainer, Cooldown::eResourceContainer>
{
  using Base = CommonModulesManager<ResourceContainer, Cooldown::eResourceContainer>;

public:
  uint16_t getStagesCount() { return Base::getStagesCount() + 1; }

protected:
  virtual bool prepareAdditionalStage(uint16_t nStageId,
                                      uint32_t nIntervalUs,
                                      uint64_t nNowUs);

  virtual void proceedAdditionalStage(uint16_t nStageId,
                                      uint32_t nIntervalUs,
                                      uint64_t nNowUs);

private:
  std::atomic_size_t m_nNextId;
};

using ResourceContainerManagerPtr = std::shared_ptr<ResourceContainerManager>;

} // namespace modules
