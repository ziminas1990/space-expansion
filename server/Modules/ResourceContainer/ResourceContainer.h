#pragma once

#include <memory>
#include <vector>
#include <atomic>

#include <Modules/BaseModule.h>
#include <Utils/GlobalContainer.h>
#include <Utils/YamlForwardDeclarations.h>
#include <Utils/UnorderedVector.h>
#include <Protocol.pb.h>
#include <World/Resources.h>
#include <Utils/SimplePool.h>

namespace modules {

class ResourceContainer :
    public BaseModule,
    public utils::GlobalContainer<ResourceContainer>
{
public:
  static std::string const& TypeName() {
    const static std::string sTypeName = "ResourceContainer";
    return sTypeName;
  }

  ResourceContainer(std::string&& sName, world::PlayerWeakPtr pOwner, uint32_t nVolume);

  // override from BaseModule
  bool loadState(YAML::Node const& data) override;
  void proceed(uint32_t nIntervalUs) override;
  void onSessionClosed(uint32_t nSessionId) override;

  void sendUpdatesIfRequired();
  // If container's content has changed since previously update,
  // then send update to each monitoring channel

  double putResource(world::Resource::Type type, double amount);
  double putResource(world::ResourcesArray const& resources);
  // Return an amount of resources, that have been actually put

  double consume(world::Resource::Type type, double amount);
  bool   consumeExactly(world::Resource::Type type, double amount);
  bool   consumeExactly(world::ResourcesArray const& resources, double maxError = 0.01);

  world::ResourcesArray const& getResources() const { return m_amount; }

private:
  void handleResourceContainerMessage(
      uint32_t nTunnelId, spex::IResourceContainer const& message) override;

  uint32_t selfId() const { return GlobalContainer<ResourceContainer>::getInstanceId(); }

  void sendOpenPortFailed(uint32_t nTunnelId, spex::IResourceContainer::Status reason);
  void sendClosePortStatus(uint32_t nTunnelId, spex::IResourceContainer::Status status);
  bool sendContent(uint32_t nTunnelId);
  void sendTransferStatus(uint32_t nTunnelId, spex::IResourceContainer::Status status);
  void sendTransferReport(uint32_t nTunnelId, world::Resource::Type type, double amount);

  void terminateActiveTransfer(spex::IResourceContainer::Status status);

  void openPort(uint32_t nTunnelId, uint32_t nAccessKey);
  void closePort(uint32_t nTunnelId);
  void transfer(uint32_t nTunnelId, spex::IResourceContainer::Transfer const& req);
  void monitor(uint32_t nTunnelId);

  double consumeResource(world::Resource::Type type, double amount);
  void recalculateUsedSpace();

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
             world::Resource::Type eResourceType, double nAmount)
      : m_nTunnelId(nTunnelId), m_nPortId(nPortId), m_nPortSecretKey(nPortSecretKey),
        m_eResourceType(eResourceType), m_nLeft(nAmount)
    {}

    bool isValid() const { return m_nPortId != m_freePortsIds.getInvalidValue(); }

    void reset() {
      m_nPortId        = m_freePortsIds.getInvalidValue();
      m_nPortSecretKey = 0;
      m_eResourceType  = world::Resource::eUnknown;
      m_nLeft          = 0;
      m_nReserved      = 0;
      m_nTransferred   = 0;
    }

    uint32_t  m_nTunnelId      = 0;
    uint32_t  m_nPortId        = m_freePortsIds.getInvalidValue();
    uint32_t  m_nPortSecretKey = 0;

    world::Resource::Type m_eResourceType = world::Resource::eUnknown;
    double    m_nLeft        = 0;
    double    m_nReserved    = 0;
    double    m_nTransferred = 0;
  };

private:
  mutable std::mutex    m_accessMutex;
  uint32_t              m_nVolume;
  double                m_nUsedSpace;
  world::ResourcesArray m_amount;

  // Monitoring
  utils::UnorderedVector<uint32_t> m_monitoringSessions;
  bool                             m_lModifiedFlag;

  // Active transferring session (from this container to another)
  uint32_t m_nOpenedPortId;
  Transfer m_activeTransfer;
  uint32_t m_nProceededTime;

  // All opened ports
  static std::mutex                     m_portsMutex; // shared_mutex?
  static utils::SimplePool<uint32_t, 0> m_freePortsIds;
  static uint32_t                       m_nNextSecretKey;
  static std::vector<Port>              m_allPorts;
};

using ResourceContainerPtr = std::shared_ptr<ResourceContainer>;

} // namespace modules
