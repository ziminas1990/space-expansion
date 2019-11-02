#include "ResourceContainer.h"
#include <Utils/YamlReader.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::ResourceContainer);

namespace modules {

ResourceContainer::ResourceContainer(uint32_t nVolume)
  : BaseModule("ResourceContainer"), m_nVolume(nVolume), m_nUsedSpace(0),
    m_amount(world::Resource::eTotalResources)
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


} // namespace modules
