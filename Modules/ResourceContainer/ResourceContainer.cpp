#include "ResourceContainer.h"
#include <Utils/YamlReader.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::ResourceContainer);

namespace modules {

std::mutex                           ResourceContainer::m_portsMutex;
utils::SimplePool<uint32_t, 0>       ResourceContainer::m_freePortsIds;
std::vector<ResourceContainer::Port> ResourceContainer::m_allPorts =
    std::vector<ResourceContainer::Port>(1024);

ResourceContainer::ResourceContainer(uint32_t nVolume)
  : BaseModule("ResourceContainer"), m_nVolume(nVolume), m_nUsedSpace(0),
    m_amount(world::Resource::eTotalResources),
    m_nOpenedPortId(m_freePortsIds.getInvalidValue())
{
  GlobalContainer<ResourceContainer>::registerSelf(this);
}

bool ResourceContainer::loadState(YAML::Node const& data)
{
  static std::vector<std::pair<std::string, world::Resource::Type> > resources = {
    std::make_pair("mettals",   world::Resource::eMettal),
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
    default:
      return;
  }
}

void ResourceContainer::sendContent(uint32_t nTunnelId)
{
  static std::vector<std::pair<world::Resource::Type, spex::ResourceType> > types = {
    {world::Resource::eMettal,   spex::ResourceType::RESOURCE_METTALS},
    {world::Resource::eIce,      spex::ResourceType::RESOURCE_ICE},
    {world::Resource::eSilicate, spex::ResourceType::RESOURCE_SILICATES}
  };

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
  response.mutable_resource_container()->mutable_error()->set_code(error);
  sendToClient(nTunnelId, response);
}

void ResourceContainer::openPort(uint32_t nTunnelId, uint32_t nAccessKey)
{
  if (m_nOpenedPortId != m_freePortsIds.getInvalidValue()) {
    sendError(nTunnelId, spex::IResourceContainer::ERROR_PORT_ALREADY_OPEN);
    return;
  }

  m_nOpenedPortId = m_freePortsIds.getInvalidValue();
  {
    std::lock_guard<std::mutex> guard(m_portsMutex);
    m_nOpenedPortId = m_freePortsIds.getNext();
    if (!m_freePortsIds.isValid(m_nOpenedPortId)) {
      sendError(nTunnelId, spex::IResourceContainer::ERROR_INTERNAL);
      return;
    }

    Port port(m_nOpenedPortId, nAccessKey);

    if (m_allPorts.size() < m_nOpenedPortId) {
      assert(m_allPorts.size() == m_nOpenedPortId);
      m_allPorts.push_back(port);
    } else {
      assert(!m_allPorts[m_nOpenedPortId].isValid());
      m_allPorts[m_nOpenedPortId] = port;
    }
  }

  spex::Message response;
  spex::IResourceContainer::PortOpened *pBody =
      response.mutable_resource_container()->mutable_port_opened();
  pBody->set_port_id(m_nOpenedPortId);
  pBody->set_access_key(nAccessKey);
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
  response.mutable_resource_container()->mutable_port_closed();
  sendToClient(nTunnelId, response);
}


} // namespace modules
