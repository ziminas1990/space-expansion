#include "ArbitratorsFactory.h"

#include <assert.h>

#include "ResourceCollecting.h"
#include <Utils/YamlReader.h>

namespace arbitrator {

BaseArbitratorPtr Factory::make(YAML::Node const& configuration,
                                world::PlayerStoragePtr pPlayersStorage)
{
  utils::YamlReader reader(configuration);
  std::string sArbitratorName;
  if (!reader.read("name", sArbitratorName)) {
    assert("Failed to read arbitrator's name" == nullptr);
    return BaseArbitratorPtr();
  }

  BaseArbitratorPtr pArbitrator;
  if (sArbitratorName == ResourceCollecting::Name()) {
    pArbitrator = std::make_shared<ResourceCollecting>(pPlayersStorage);
  }

  if (!pArbitrator || !pArbitrator->loadConfiguation(configuration)) {
    assert("Failed to create arbitrator" == nullptr);
    return BaseArbitratorPtr();
  }

  return pArbitrator;
}


} // namespace arbitrator
