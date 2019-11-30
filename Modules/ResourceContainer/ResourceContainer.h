#pragma once

#include <memory>
#include <vector>
#include <atomic>

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

  // override from BaseModule
  bool loadState(YAML::Node const& data) override;
  void proceed(uint32_t nIntervalUs) override;

  double putResource(world::Resources::Type type, double amount);
  // Return an amount of resources, that have been actually put
  double consume(world::Resources::Type type, double amount);
  bool   consumeExactly(world::Resources::Type type, double amount);

private:
  void handleResourceContainerMessage(
      uint32_t nTunnelId, spex::IResourceContainer const& message) override;

  uint32_t selfId() const { return GlobalContainer<ResourceContainer>::getInstanceId(); }

  void sendContent(uint32_t nTunnelId);
  void sendError(uint32_t nTunnelId, spex::IResourceContainer::Error error);
  void sendTransferReport(uint32_t nTunnelId, world::Resources::Type type, double amount);
  void sendTransferComplete(uint32_t nTunnelId);
  void sendTransferFailed(uint32_t nTunnelId, spex::IResourceContainer::Error reason);

  void onTransferFailed(spex::IResourceContainer::Error reason);
  void terminateActiveTransfer(bool sendCompleteInd = true);

  void openPort(uint32_t nTunnelId, uint32_t nAccessKey);
  void closePort(uint32_t nTunnelId);
  void transfer(uint32_t nTunnelId, spex::IResourceContainer::Transfer const& req);

  double consumeResource(world::Resources::Type type, double amount);

private:

  struct Port {
    Port() = default;
    Port(uint32_t nAccessKey, uint32_t nSecretKey, uint32_t nContainerId)
      : m_nAccessKey(nAccessKey), m_nSecretKey(nSecretKey), m_nContainerId(nContainerId)
    {
      assert(nSecretKey > 0);
    }

    bool isValid() const { return m_nSecretKey > 0; }
  
    uint32_t m_nAccessKey   = 0;
    uint32_t m_nSecretKey   = 0;
    uint32_t m_nContainerId = 0;
  };
  
  struct Transfer {
    Transfer() = default;
    Transfer(uint32_t nTunnelId, uint32_t nPortId, uint32_t nPortSecretKey,
             world::Resources::Type eResourceType, double nAmount)
      : m_nTunnelId(nTunnelId), m_nPortId(nPortId), m_nPortSecretKey(nPortSecretKey),
        m_eResourceType(eResourceType), m_nLeft(nAmount)
    {}

    bool isValid() const { return m_nPortId != m_freePortsIds.getInvalidValue(); }

    void reset() {
      m_nPortId        = m_freePortsIds.getInvalidValue();
      m_nPortSecretKey = 0;
      m_eResourceType  = world::Resources::eUnknown;
      m_nLeft          = 0;
      m_nReserved      = 0;
      m_nTransferred   = 0;
    }

    uint32_t  m_nTunnelId      = 0;
    uint32_t  m_nPortId        = m_freePortsIds.getInvalidValue();
    uint32_t  m_nPortSecretKey = 0;

    world::Resources::Type m_eResourceType = world::Resources::eUnknown;
    double    m_nLeft        = 0;
    double    m_nReserved    = 0;
    double    m_nTransferred = 0;
  };

private:
  mutable std::mutex  m_accessMutex;
  uint32_t            m_nVolume;
  double              m_nUsedSpace;
  std::vector<double> m_amount;

  uint32_t m_nOpenedPortId;
  Transfer m_activeTransfer;

  static std::mutex                     m_portsMutex; // shared_mutex?
  static utils::SimplePool<uint32_t, 0> m_freePortsIds;
  static uint32_t                       m_nNextSecretKey;
  static std::vector<Port>              m_allPorts;
};


} // namespace modules
