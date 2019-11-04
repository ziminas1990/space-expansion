#include "ClientResourceContainer.h"

namespace autotests { namespace client {

static world::Resource::Type convert(spex::ResourceType type)
{
  switch (type) {
    case spex::ResourceType::RESOURCE_METTALS:
      return world::Resource::eMettal;
    case spex::ResourceType::RESOURCE_SILICATES:
      return world::Resource::eSilicate;
    case spex::ResourceType::RESOURCE_ICE:
      return world::Resource::eIce;
    default:
      assert(false);
      return world::Resource::eMettal;  // ?
  }
}

static ResourceContainer::Status convert(spex::IResourceContainer::Error error)
{
  switch (error) {
    case spex::IResourceContainer::ERROR_PORT_ALREADY_OPEN:
      return ResourceContainer::eStatusPortAlreadyOpen;
    default: {
      assert(false);
      return ResourceContainer::eStatusError;
    }
  }
}

static bool fillContent(ResourceContainer::Content &content,
                        spex::IResourceContainer::Content const& data)
{
  content.m_nVolume    = data.volume();
  content.m_nUsedSpace = data.used();

  for (spex::ResourceItem const& item : data.resources()) {
    world::Resource::Type eType = convert(item.type());
    content.m_amount[eType] = item.amount();
  }
  return true;
}

bool ResourceContainer::getContent(ResourceContainer::Content &content)
{
  spex::Message request;
  request.mutable_resource_container()->mutable_get_content();
  if (!send(request))
    return false;

  spex::IResourceContainer response;
  if (!wait(response))
    return false;
  if (response.choice_case() != spex::IResourceContainer::kContent)
    return false;

  return fillContent(content, response.content());
}

ResourceContainer::Status ResourceContainer::openPort(
    uint32_t nAccessKey, uint32_t& nPortId)
{
  spex::Message request;
  request.mutable_resource_container()->mutable_open_port()->set_access_key(nAccessKey);

  if (!send(request))
    return eStatusError;

  spex::IResourceContainer response;
  if (!wait(response))
    return eStatusError;
  if (response.choice_case() == spex::IResourceContainer::kError) {
    return convert(response.error().code());
  }

  if (response.choice_case() != spex::IResourceContainer::kPortOpened) {
    return eStatusError;
  }

  nPortId = response.port_opened().port_id();
  return eStatusOk;
}


}}  // namespace autotests::client
