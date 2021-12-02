#include "ResourceContainer.h"
#include <Utils/YamlReader.h>
#include <Utils/FloatComparator.h>
#include <Utils/ItemsConverter.h>

#include <Ships/Ship.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::ResourceContainer);

namespace modules {


//==============================================================================
// ResourceContainer
//==============================================================================

std::mutex                           ResourceContainer::m_portsMutex;
utils::SimplePool<uint32_t, 0>       ResourceContainer::m_freePortsIds;
std::vector<ResourceContainer::Port> ResourceContainer::m_allPorts =
    std::vector<ResourceContainer::Port>(1024);
uint32_t                             ResourceContainer::m_nNextSecretKey = 1;

ResourceContainer::ResourceContainer(
    std::string &&sName, world::PlayerWeakPtr pOwner, uint32_t nVolume)
  : BaseModule("ResourceContainer", std::move(sName), std::move(pOwner)),
    m_nVolume(nVolume),
    m_nUsedSpace(0),
    m_lModifiedFlag(false),
    m_nOpenedPortId(m_freePortsIds.getInvalidValue())
{
  GlobalObject<ResourceContainer>::registerSelf(this);
  m_amount.fill(0);
}

bool ResourceContainer::loadState(YAML::Node const& data)
{
  m_amount.load(data);
  m_nUsedSpace = m_amount.calculateTotalVolume();
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

  assert(port.m_nContainerId <
         utils::GlobalContainer<ResourceContainer>::Total());
  ResourceContainer* pReceiver =
      utils::GlobalContainer<ResourceContainer>::Instance(port.m_nContainerId);

  const double distanceToReceiver =
      getPlatform()->getDistanceTo(pReceiver->getPlatform());
  if (distanceToReceiver > 200.0) {
    terminateActiveTransfer(spex::IResourceContainer::PORT_TOO_FAR);
    switchToIdleState();
    return;
  }

  const double nIntervalSec = nIntervalUs / 1000000.0;
  double transferedAmount   = weightPerSecond * nIntervalSec;
  if (transferedAmount > m_activeTransfer.m_nLeft)
    transferedAmount = m_activeTransfer.m_nLeft;

  m_activeTransfer.m_nReserved +=
      consumeResource(m_activeTransfer.m_eResourceType,
                      transferedAmount - m_activeTransfer.m_nReserved);

  const double actuallyTransfered =
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

void ResourceContainer::onSessionClosed(uint32_t nSessionId)
{
  m_monitoringSessions.removeFirst(nSessionId);
}

void ResourceContainer::sendUpdatesIfRequired()
{
  if (m_lModifiedFlag) {
    m_lModifiedFlag = false;
    for (size_t i = 0; i < m_monitoringSessions.size(); ++i) {
      if (!sendContent(m_monitoringSessions[i])) {
        m_monitoringSessions.remove(i);
      }
    }
  }
}

double ResourceContainer::putResource(world::Resource::Type type, double amount)
{
  double transfferedVolume = amount / world::Resource::Density[type];

  std::lock_guard<std::mutex> guard(m_accessMutex);
  double freeVolume = m_nVolume - m_nUsedSpace;
  if (freeVolume < transfferedVolume) {
    transfferedVolume = freeVolume;
    amount            = freeVolume * world::Resource::Density[type];
  }

  m_amount[type] += amount;
  m_nUsedSpace   += transfferedVolume;
  m_lModifiedFlag = true;
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
    consumeResource(type, amount);
    return true;
  } else {
    return false;
  }
}

bool ResourceContainer::consumeExactly(world::ResourcesArray const& resources,
                                       double maxError)
{
  std::lock_guard<std::mutex> guard(m_accessMutex);
  for (world::Resource::Type eResource : world::Resource::MaterialResources) {
    if (m_amount[eResource] + maxError < resources[eResource])
      return false;
  }
  for (world::Resource::Type eResource : world::Resource::MaterialResources) {
    assert(m_amount[eResource] + maxError >= resources[eResource]);
    m_amount[eResource] -= resources[eResource];
    if (m_amount[eResource] < 0) {
      assert(m_amount[eResource] > -maxError);
      m_amount[eResource] = 0;
    }
  }
  m_lModifiedFlag = true;
  recalculateUsedSpace();
  return true;
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
      openPort(nTunnelId, message.open_port());
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
    case spex::IResourceContainer::kMonitor: {
      monitor(nTunnelId);
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

bool ResourceContainer::sendContent(uint32_t nTunnelId)
{
  using TypesPair = std::pair<world::Resource::Type, spex::ResourceType>;
  static const std::vector<TypesPair> types =
  {
    {world::Resource::eMetal,    spex::ResourceType::RESOURCE_METALS},
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
  return sendToClient(nTunnelId, response);
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

  world::Resource::Type eType = utils::convert(req.resource().type());
  if (!world::Resource::isMaterial(eType)) {
    sendTransferStatus(nTunnelId, spex::IResourceContainer::INVALID_RESOURCE_TYPE);
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
        Transfer(nTunnelId, req.port_id(), port.m_nSecretKey, eType,
                 req.resource().amount());
  }

  sendTransferStatus(nTunnelId, spex::IResourceContainer::SUCCESS);
  switchToActiveState();
}

void ResourceContainer::monitor(uint32_t nTunnelId)
{
  m_monitoringSessions.push(nTunnelId);
  sendContent(nTunnelId);
}

double ResourceContainer::consumeResource(world::Resource::Type type, double amount)
{
  amount = std::min(m_amount[type], amount);
  m_nUsedSpace   -= amount / world::Resource::Density[type];
  m_amount[type] -= amount;

  if (m_nUsedSpace < 0)
    m_nUsedSpace = 0;
  if (m_amount[type] < 0)
    m_amount[type] = 0;
  m_lModifiedFlag = true;
  return amount;
}

void ResourceContainer::recalculateUsedSpace()
{
  double usedSpace = 0;
  for (size_t type = 0; type < world::Resource::eLabor; ++type) {
    usedSpace += m_amount[type] / world::Resource::Density[type];
  }
  assert(usedSpace <= m_nVolume);
  m_nUsedSpace = usedSpace;
}

} // namespace modules
