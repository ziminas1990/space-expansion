#pragma once
#include "BaseArbitrator.h"
#include <World/Resources.h>

namespace arbitrator {

class ResourceCollecting : public BaseArbitrator
{
public:
  static std::string const& Name() {
    static const std::string sName = "Collecting Resources";
    return sName;
  }

  ResourceCollecting(world::PlayerStoragePtr pPlayersStorage);

  bool loadConfiguation(YAML::Node const& data) override;

protected:
  uint32_t score(world::PlayerPtr pPlayer) override;

private:
  world::ResourcesArray m_target;
};

} // namespace arbitrator
