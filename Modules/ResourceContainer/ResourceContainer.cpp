#include "ResourceContainer.h"
#include <Utils/YamlReader.h>
#include <Utils/FloatComparator.h>
#include <Utils/ItemsConverter.h>

#include <Ships/Ship.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::ResourceContainer);

namespace modules {


//========================================================================================
// ResourceContainer
//========================================================================================

std::mutex                           ResourceContainer::m_portsMutex;
utils::SimplePool<uint32_t, 0>       ResourceContainer::m_freePortsIds;
std::vector<ResourceContainer::Port> ResourceContainer::m_allPorts =
    std::vector<ResourceContainer::Port>(1024);
uint32_t                             ResourceContainer::m_nNextSecretKey = 1;

ResourceContainer::ResourceContainer(std::string &&sName, uint32_t nVolume)
  : BaseModule("ResourceContainer", std::move(sName)),
    m_nVolume(nVolume), m_nUsedSpace(0), m_amount(world::Resource::eTotalResources),
    m_nOpenedPortId(m_freePortsIds.getInvalidValue())
{
  GlobalContainer<ResourceContainer>::registerSelf(this);
}

bool ResourceContainer::loadState(YAML::Node const& data)
{
  static std::vector<std::pair<std::string, world::Resource::Type> > resources = {
    std::make_pair("mettals",   world::Resource::eMetal),
    std::make_pair("silicates", world::Resource::eSilicate),
    std::make_pair("ice",       world::Resource::eIce),
  };

  if (!BaseModule::loadState(data))
    return false;

  m_nUsedSpace = 0;

  utils::YamlReader reader(data);
  for (auto resource : resources) {
    m_amount[resource.second] = 0;
    reader.read(resource.first.c_str(), m_amount[resource.second]);
    m_nUsedSpace += m_amount[resource.second] / world::Resource::density[resource.second];
  }

  return m_nUsedSpace <= m_nVolume;
}

void ResourceContainer::proceed(uint32_t nIntervalUs)
{
  const double weightPerSecond = 2000;

  if (!nIntervalUs)
    return;

  if (!m_activeTransfer.isValid()) {
    switchToIdleState();
    return;
  }

  assert(m_activeTransfer.m_nPortId < m_allPorts.size());
  Port& port = m_allPorts[m_activeTransfer.m_nPortId];

  if (!port.isValid() || m_activeTransfer.m_nPortSecretKey != port.m_nSecretKey) {
    terminateActiveTransfer(spex::IResourceContainer::PORT_HAS_BEEN_CLOSED);
    switchToIdleState();
    return;
  }

  assert(port.m_nContainerId < GlobalContainer<ResourceContainer>::TotalInstancies());
  ResourceContainer* pReceiver =
      GlobalContainer<ResourceContainer>::Instance(port.m_nContainerId);

  double distanceToReceiver = getPlatform()->getDistanceTo(pReceiver->getPlatform());
  if (distanceToReceiver > 200.0) {
    terminateActiveTransfer(spex::IResourceContainer::PORT_TOO_FAR);
    switchToIdleState();
    return;
  }

  double nIntervalSec     = nIntervalUs / 1000000.0;
  double transferedAmount = weightPerSecond * nIntervalSec;
  if (transferedAmount > m_activeTransfer.m_nLeft)
    transferedAmount = m_activeTransfer.m_nLeft;

  m_activeTransfer.m_nReserved +=
      consumeResource(m_activeTransfer.m_eResourceType,
                      transferedAmount - m_activeTransfer.m_nReserved);

  double actuallyTransfered =
      pReceiver->putResource(m_activeTransfer.m_eResourceType,
                             m_activeTransfer.m_nReserved);
  m_activeTransfer.m_nReserved -= actuallyTransfered;

  sendTransferReport(m_activeTransfer.m_nTunnelId, m_activeTransfer.m_eResourceType,
                     actuallyTransfered);

  m_activeTransfer.m_nLeft -= actuallyTransfered;
  if (utils::AlmostEqual(m_activeTransfer.m_nLeft, 0)) {
    terminateActiveTransfer(spex::IResourceContainer::SUCCESS);
    switchToIdleState();
  }
}

double ResourceContainer::putResource(world::Resource::Type type, double amount)
{
  double transfferedVolume = amount / world::Resource::density[type];

  std::lock_guard<std::mutex> guard(m_accessMutex);
  double freeVolume = m_nVolume - m_nUsedSpace;
  if (freeVolume < transfferedVolume) {
    transfferedVolume = freeVolume;
    amount            = freeVolume * world::Resource::density[type];
  }

  m_amount[type] += amount;
  m_nUsedSpace   += transfferedVolume;
  return amount;
}

double ResourceContainer::consume(world::Resource::Type type, double amount)
{
  std::lock_guard<std::mutex> guard(m_accessMutex);
  return consumeResource(type, amount);
}

bool ResourceContainer::consumeExactly(world::Resource::Type type, double amount)
{
  std::lock_guard<std::mutex> guard(m_accessMutex);
  if (m_amount[type] >= amount) {
    consume(type, amount);
    return true;
  } else {
    return false;
  }
}

void ResourceContainer::handleResourceContainerMessage(
    uint32_t nTunnelId, spex::IResourceContainer const & message)
{
  switch(message.choice_case()) {
    case spex::IResourceContainer::kContentReq: {
      sendContent(nTunnelId);
      return;
    }
    case spex::IResourceContainer::kOpenPort: {
      openPort(nTunnelId, message.open_port().access_key());
      return;
    }
    case spex::IResourceContainer::kClosePort: {
      closePort(nTunnelId);
      return;
    }
    case spex::IResourceContainer::kTransfer: {
      transfer(nTunnelId, message.transfer());
      return;
    }
    default:
      return;
  }
}

void ResourceContainer::sendOpenPortFailed(
    uint32_t nTunnelId, spex::IResourceContainer::Status reason)
{
  spex::Message response;
  response.mutable_resource_container()->set_open_port_failed(reason);
  sendToClient(nTunnelId, response);
}

void ResourceContainer::sendClosePortStatus(
    uint32_t nTunnelId, spex::IResourceContainer::Status status)
{
  spex::Message response;
  response.mutable_resource_container()->set_close_port_status(status);
  sendToClient(nTunnelId, response);
}

void ResourceContainer::sendContent(uint32_t nTunnelId)
{
  static std::vector<std::pair<world::Resource::Type, spex::ResourceType> > types = {
    {world::Resource::eMetal,   spex::ResourceType::RESOURCE_METALS},
    {world::Resource::eIce,      spex::ResourceType::RESOURCE_ICE},
    {world::Resource::eSilicate, spex::ResourceType::RESOURCE_SILICATES}
  };

  // Nobody wouldn't change container's content during command handling phase,
  // so there is no need to lock mutex

  spex::Message response;
  spex::IResourceContainer::Content* pBody =
      response.mutable_resource_container()->mutable_content();

  for (auto resourceType : types) {
    if (m_amount[resourceType.first] > 0) {
      spex::ResourceItem *pItem = pBody->add_resources();
      pItem->set_type(resourceType.second);
      pItem->set_amount(m_amount[resourceType.first]);
    }
  }
  pBody->set_used(m_nUsedSpace);
  pBody->set_volume(m_nVolume);
  sendToClient(nTunnelId, response);
}

void ResourceContainer::sendTransferStatus(
    uint32_t nTunnelId, spex::IResourceContainer::Status status)
{
  spex::Message response;
  response.mutable_resource_container()->set_transfer_status(status);
  sendToClient(nTunnelId, response);
}


void ResourceContainer::sendTransferReport(
    uint32_t nTunnelId, world::Resource::Type type, double amount)
{
  spex::Message message;
  spex::ResourceItem *pReport =
      message.mutable_resource_container()->mutable_transfer_report();
  pReport->set_type(utils::convert(type));
  pReport->set_amount(amount);
  sendToClient(nTunnelId, message);
}

void ResourceContainer::terminateActiveTransfer(spex::IResourceContainer::Status status)
{
  if (m_activeTransfer.m_nReserved > 0) {
    // If container is full already, reserved resources will be lost!
    putResource(m_activeTransfer.m_eResourceType, m_activeTransfer.m_nReserved);
  }
  spex::Message message;
  message.mutable_resource_container()->set_transfer_finished(status);
  sendToClient(m_activeTransfer.m_nTunnelId, message);
  m_activeTransfer = Transfer();
}

void ResourceContainer::openPort(uint32_t nTunnelId, uint32_t nAccessKey)
{
  if (m_nOpenedPortId != m_freePortsIds.getInvalidValue()) {
    m_nOpenedPortId = m_freePortsIds.getInvalidValue();
    sendOpenPortFailed(nTunnelId, spex::IResourceContainer::PORT_ALREADY_OPEN);
    return;
  }

  {
    std::lock_guard<std::mutex> guard(m_portsMutex);
    m_nOpenedPortId = m_freePortsIds.getNext();
    if (!m_freePortsIds.isValid(m_nOpenedPortId)) {
      sendOpenPortFailed(nTunnelId, spex::IResourceContainer::INTERNAL_ERROR);
      return;
    }

    Port newPort(nAccessKey, m_nNextSecretKey++, selfId());

    if (m_allPorts.size() < m_nOpenedPortId) {
      assert(m_allPorts.size() == m_nOpenedPortId);
      m_allPorts.push_back(newPort);
    } else {
      assert(!m_allPorts[m_nOpenedPortId].isValid());
      m_allPorts[m_nOpenedPortId] = newPort;
    }
  }

  spex::Message response;
  response.mutable_resource_container()->set_port_opened(m_nOpenedPortId);
  sendToClient(nTunnelId, response);
}

void ResourceContainer::closePort(uint32_t nTunnelId)
{
  if (m_nOpenedPortId == m_freePortsIds.getInvalidValue()) {
    sendClosePortStatus(nTunnelId, spex::IResourceContainer::PORT_IS_NOT_OPENED);
    return;
  }

  {
    std::lock_guard<std::mutex> guard(m_portsMutex);
    m_allPorts[m_nOpenedPortId] = Port();
    m_freePortsIds.release(m_nOpenedPortId);
    m_nOpenedPortId = m_freePortsIds.getInvalidValue();
  }

  sendClosePortStatus(nTunnelId, spex::IResourceContainer::SUCCESS);
}

void ResourceContainer::transfer(
    uint32_t nTunnelId, spex::IResourceContainer::Transfer const& req)
{
  if (m_activeTransfer.isValid()) {
    sendTransferStatus(nTunnelId, spex::IResourceContainer::TRANSFER_IN_PROGRESS);
    return;
  }

  {
    std::lock_guard<std::mutex> guard(m_portsMutex);

    if (req.port_id() == m_freePortsIds.getInvalidValue() ||
        req.port_id() >= m_allPorts.size()) {
      sendTransferStatus(nTunnelId, spex::IResourceContainer::PORT_IS_NOT_OPENED);
      return;
    }

    Port& port = m_allPorts[req.port_id()];
    if (!port.isValid()) {
      sendTransferStatus(nTunnelId, spex::IResourceContainer::PORT_IS_NOT_OPENED);
      return;
    }
    if (port.m_nAccessKey != req.access_key()) {
      sendTransferStatus(nTunnelId, spex::IResourceContainer::INVALID_ACCESS_KEY);
      return;
    }

    m_activeTransfer =
        Transfer(nTunnelId, req.port_id(), port.m_nSecretKey,
                 utils::convert(req.resource().type()), req.resource().amount());
  }

  sendTransferStatus(nTunnelId, spex::IResourceContainer::SUCCESS);
  switchToActiveState();
}

double ResourceContainer::consumeResource(world::Resource::Type type, double amount)
{
  amount = std::min(m_amount[type], amount);
  m_nUsedSpace   -= amount / world::Resource::density[type];
  m_amount[type] -= amount;

  if (m_nUsedSpace < 0)
    m_nUsedSpace = 0;
  if (m_amount[type] < 0)
    m_amount[type] = 0;
  return amount;
}

} // namespace modules
