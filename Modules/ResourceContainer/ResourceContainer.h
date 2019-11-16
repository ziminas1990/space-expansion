#pragma once

#include <memory>
#include <vector>

#include <Modules/BaseModule.h>
#include <Utils/GlobalContainer.h>
#include <Utils/YamlForwardDeclarations.h>
#include <Protocol.pb.h>
#include <World/Resources.h>
#include <Utils/SimplePool.h>

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
  void sendError(uint32_t nTunnelId, spex::IResourceContainer::Error error);

  void openPort(uint32_t nTunnelId, uint32_t nAccessKey);
  void closePort(uint32_t nTunnelId);

private:
  uint32_t m_nVolume;

  double m_nUsedSpace;
  std::vector<double> m_amount;

  struct Port {
    Port() = default;
    Port(uint32_t nPortId, uint32_t nAccessKey)
      : m_nPortId(nPortId), m_nAccessKey(nAccessKey)
    {}

    bool isValid() const { return m_nPortId > 0; }
  
    uint32_t m_nPortId    = 0;
    uint32_t m_nAccessKey = 0;
  };
  


  uint32_t m_nOpenedPortId;

  static std::mutex                     m_portsMutex; // shared_mutex?
  static utils::SimplePool<uint32_t, 0> m_freePortsIds;
  static std::vector<Port>              m_allPorts;
};

} // namespace modules
