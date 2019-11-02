#pragma once

#include <memory>
#include <Modules/BaseModule.h>
#include <Utils/GlobalContainer.h>
#include <Utils/YamlForwardDeclarations.h>
#include <Protocol.pb.h>
#include <World/Resources.h>

namespace modules {

class ResourceContainer :
    public BaseModule,
    public utils::GlobalContainer<ResourceContainer>
{
public:
  ResourceContainer(uint32_t nVolume);

  bool loadState(YAML::Node const& data) override;

private:
  void handleResourceContainerMessage(
      uint32_t nTunnelId, spex::IResourceContainer const& message) override;

  void sendContent(uint32_t nTunnelId);

private:
  uint32_t m_nVolume;

  double m_nUsedSpace;
  std::vector<double> m_amount;
};

} // namespace modules
