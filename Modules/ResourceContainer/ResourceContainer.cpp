#include "ResourceContainer.h"
#include <Utils/YamlReader.h>
#include <Utils/FloatComparator.h>

#include <Ships/Ship.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::ResourceContainer);

namespace modules {

//========================================================================================
// Helpers
//========================================================================================

static world::Resources::Type convert(spex::ResourceType type)
{
  switch (type) {
    case spex::ResourceType::RESOURCE_ICE:
      return world::Resources::eIce;
    case spex::ResourceType::RESOURCE_METTALS:
      return world::Resources::eMettal;
    case spex::ResourceType::RESOURCE_SILICATES:
      return world::Resources::eSilicate;
    default:
      return world::Resources::eUnknown;
  }
}

static spex::ResourceType convert(world::Resources::Type type)
{
  switch (type) {
    case world::Resources::eIce:
      return spex::ResourceType::RESOURCE_ICE;
    case world::Resources::eMettal:
      return spex::ResourceType::RESOURCE_METTALS;
    case world::Resources::eSilicate:
      return spex::ResourceType::RESOURCE_SILICATES;
    default:
      assert(false);
      return spex::ResourceType::RESOURCE_ICE;
  }
}


//========================================================================================
// ResourceContainer
//========================================================================================

std::mutex                           ResourceContainer::m_portsMutex;
utils::SimplePool<uint32_t, 0>       ResourceContainer::m_freePortsIds;
std::vector<ResourceContainer::Port> ResourceContainer::m_allPorts =
    std::vector<ResourceContainer::Port>(1024);
uint32_t                             ResourceContainer::m_nNextSecretKey = 1;

ResourceContainer::ResourceContainer(uint32_t nVolume)
  : BaseModule("ResourceContainer"), m_nVolume(nVolume), m_nUsedSpace(0),
    m_amount(world::Resources::eTotalResources),
    m_nOpenedPortId(m_freePortsIds.getInvalidValue())
{
  GlobalContainer<ResourceContainer>::registerSelf(this);
}

bool ResourceContainer::loadState(YAML::Node const& data)
{
  static std::vector<std::pair<std::string, world::Resources::Type> > resources = {
    std::make_pair("mettals",   world::Resources::eMettal),
    std::make_pair("silicates", world::Resources::eSilicate),
    std::make_pair("ice",       world::Resources::eIce),
  };

  if (!BaseModule::loadState(data))
    return false;

  m_nUsedSpace = 0;

  utils::YamlReader reader(data);
  for (auto resource : resources) {
    m_amount[resource.second] = 0;
    reader.read(resource.first.c_str(), m_amount[resource.second]);
    m_nUsedSpace += m_amount[resource.second] / world::Resources::density[resource.second];
  }

  return m_nUsedSpace <= m_nVolume;
}

void ResourceContainer::proceed(uint32_t nIntervalUs)
{
  const double weightPerSecond = 2000;

  if (!m_activeTransfer.isValid()) {
    switchToIdleState();
    return;
  }

  assert(m_activeTransfer.m_nPortId < m_allPorts.size());
  Port& port = m_allPorts[m_activeTransfer.m_nPortId];

  if (!port.isValid() || m_activeTransfer.m_nPortSecretKey != port.m_nSecretKey) {
    onTransferFailed(spex::IResourceContainer::ERROR_PORT_HAS_BEEN_CLOSED);
    switchToIdleState();
    return;
  }

  assert(port.m_nContainerId < GlobalContainer<ResourceContainer>::TotalInstancies());
  ResourceContainer* pReceiver =
      GlobalContainer<ResourceContainer>::Instance(port.m_nContainerId);

  double distanceToReceiver =
      getPlatform()->getPosition().distance(
        pReceiver->getPlatform()->getPosition());
  if (distanceToReceiver > 200.0) {
    onTransferFailed(spex::IResourceContainer::ERROR_TOO_FAR);
    switchToIdleState();
    return;
  }

  double nIntervalSec     = nIntervalUs / 1000000.0;
  double transferedAmount = weightPerSecond * nIntervalSec;
  if (transferedAmount > m_activeTransfer.m_nLeft)
    transferedAmount = m_activeTransfer.m_nLeft;

  m_activeTransfer.m_nReserved =
      m_activeTransfer.m_nReserved +
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
    terminateActiveTransfer();
    switchToIdleState();
  }
}

double ResourceContainer::putResource(world::Resources::Type type, double amount)
{
  double transfferedVolume = amount / world::Resources::density[type];

  std::lock_guard<std::mutex> guard(m_accessMutex);
  double freeVolume = m_nVolume - m_nUsedSpace;
  if (freeVolume < transfferedVolume) {
    transfferedVolume = freeVolume;
    amount = freeVolume * world::Resources::density[type];
  }

  m_amount[type] += amount;
  m_nUsedSpace   += transfferedVolume;
  return amount;
}

double ResourceContainer::consume(world::Resources::Type type, double amount)
{
  std::lock_guard<std::mutex> guard(m_accessMutex);
  return consumeResource(type, amount);
}

bool ResourceContainer::consumeExactly(world::Resources::Type type, double amount)
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
    case spex::IResourceContainer::kGetContent: {
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

void ResourceContainer::sendContent(uint32_t nTunnelId)
{
  static std::vector<std::pair<world::Resources::Type, spex::ResourceType> > types = {
    {world::Resources::eMettal,   spex::ResourceType::RESOURCE_METTALS},
    {world::Resources::eIce,      spex::ResourceType::RESOURCE_ICE},
    {world::Resources::eSilicate, spex::ResourceType::RESOURCE_SILICATES}
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

void ResourceContainer::sendError(uint32_t nTunnelId,
                                  spex::IResourceContainer::Error error)
{
  spex::Message response;
  response.mutable_resource_container()->set_on_error(error);
  sendToClient(nTunnelId, response);
}

void ResourceContainer::sendTransferReport(
    uint32_t nTunnelId, world::Resources::Type type, double amount)
{
  spex::Message message;
  spex::ResourceItem *pReport =
      message.mutable_resource_container()->mutable_transfer_report();
  pReport->set_type(convert(type));
  pReport->set_amount(amount);
  sendToClient(nTunnelId, message);
}

void ResourceContainer::sendTransferComplete(uint32_t nTunnelId)
{
  spex::Message message;
  message.mutable_resource_container()->set_transfer_complete(true);
  sendToClient(nTunnelId, message);
}

void ResourceContainer::sendTransferFailed(
    uint32_t nTunnelId, spex::IResourceContainer::Error reason)
{
  spex::Message message;
  message.mutable_resource_container()->set_transfer_failed(reason);
  sendToClient(nTunnelId, message);
}

void ResourceContainer::onTransferFailed(spex::IResourceContainer::Error reason)
{
  sendTransferFailed(m_activeTransfer.m_nTunnelId, reason);
  terminateActiveTransfer(false);
}

void ResourceContainer::terminateActiveTransfer(bool sendCompleteInd)
{
  if (m_activeTransfer.m_nReserved > 0) {
    // If container is full already, reserved resources will be lost!
    putResource(m_activeTransfer.m_eResourceType, m_activeTransfer.m_nReserved);
  }
  if (sendCompleteInd) {
    sendTransferComplete(m_activeTransfer.m_nTunnelId);
  }
  m_activeTransfer = Transfer();
}

void ResourceContainer::openPort(uint32_t nTunnelId, uint32_t nAccessKey)
{
  if (m_nOpenedPortId != m_freePortsIds.getInvalidValue()) {
    m_nOpenedPortId = m_freePortsIds.getInvalidValue();
    sendError(nTunnelId, spex::IResourceContainer::ERROR_PORT_ALREADY_OPEN);
    return;
  }

  {
    std::lock_guard<std::mutex> guard(m_portsMutex);
    m_nOpenedPortId = m_freePortsIds.getNext();
    if (!m_freePortsIds.isValid(m_nOpenedPortId)) {
      sendError(nTunnelId, spex::IResourceContainer::ERROR_INTERNAL);
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
    sendError(nTunnelId, spex::IResourceContainer::ERROR_PORT_IS_NOT_OPENED);
    return;
  }

  {
    std::lock_guard<std::mutex> guard(m_portsMutex);
    m_allPorts[m_nOpenedPortId] = Port();
    m_freePortsIds.release(m_nOpenedPortId);
    m_nOpenedPortId = m_freePortsIds.getInvalidValue();
  }

  spex::Message response;
  response.mutable_resource_container()->set_port_closed(true);
  sendToClient(nTunnelId, response);
}

void ResourceContainer::transfer(uint32_t nTunnelId,
                                 spex::IResourceContainer::Transfer const& req)
{
  if (m_activeTransfer.isValid()) {
    sendError(nTunnelId, spex::IResourceContainer::ERROR_TRANSFER_IN_PROGRESS);
    return;
  }

  {
    std::lock_guard<std::mutex> guard(m_portsMutex);

    if (req.port_id() == m_freePortsIds.getInvalidValue() ||
        req.port_id() >= m_allPorts.size()) {
      sendError(nTunnelId, spex::IResourceContainer::ERROR_PORT_IS_NOT_OPENED);
      return;
    }

    Port& port = m_allPorts[req.port_id()];
    if (!port.isValid()) {
      sendError(nTunnelId, spex::IResourceContainer::ERROR_PORT_IS_NOT_OPENED);
      return;
    }
    if (port.m_nAccessKey != req.access_key()) {
      sendError(nTunnelId, spex::IResourceContainer::ERROR_INVALID_KEY);
      return;
    }

    m_activeTransfer =
        Transfer(nTunnelId, req.port_id(), port.m_nSecretKey,
                 convert(req.resource().type()), req.resource().amount());
  }

  switchToActiveState();
}

double ResourceContainer::consumeResource(world::Resources::Type type, double amount)
{
  amount = std::min(m_amount[type], amount);
  m_nUsedSpace   -= amount / world::Resources::density[type];
  m_amount[type] -= amount;

  if (m_nUsedSpace < 0)
    m_nUsedSpace = 0;
  if (m_amount[type] < 0)
    m_amount[type] = 0;
  return amount;
}


} // namespace modules
