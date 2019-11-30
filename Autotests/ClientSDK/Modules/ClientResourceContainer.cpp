#include "ClientResourceContainer.h"

namespace autotests { namespace client {

static world::Resources::Type convert(spex::ResourceType type)
{
  switch (type) {
    case spex::ResourceType::RESOURCE_METTALS:
      return world::Resources::eMettal;
    case spex::ResourceType::RESOURCE_SILICATES:
      return world::Resources::eSilicate;
    case spex::ResourceType::RESOURCE_ICE:
      return world::Resources::eIce;
    default:
      assert(false);
      return world::Resources::eMettal;  // ?
  }
}

static ResourceContainer::Status convert(spex::IResourceContainer::Error error)
{
  switch (error) {
    case spex::IResourceContainer::ERROR_PORT_ALREADY_OPEN:
      return ResourceContainer::eStatusPortAlreadyOpen;
    case spex::IResourceContainer::ERROR_PORT_IS_NOT_OPENED:
      return ResourceContainer::eStatusPortIsNotOpened;
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
    world::Resources::Type eType = convert(item.type());
    content.m_amount[eType] = item.amount();
  }
  return true;
}

bool ResourceContainer::getContent(ResourceContainer::Content &content)
{
  spex::Message request;
  request.mutable_resource_container()->set_get_content(true);
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
  if (response.choice_case() == spex::IResourceContainer::kOnError) {
    return convert(response.on_error());
  }

  if (response.choice_case() != spex::IResourceContainer::kPortOpened) {
    return eStatusError;
  }

  nPortId = response.port_opened();
  return eStatusOk;
}

ResourceContainer::Status ResourceContainer::closePort()
{
  spex::Message request;
  request.mutable_resource_container()->set_close_port(true);

  if (!send(request))
    return eStatusError;

  spex::IResourceContainer response;
  if (!wait(response))
    return eStatusError;
  if (response.choice_case() == spex::IResourceContainer::kOnError) {
    return convert(response.on_error());
  }

  if (response.choice_case() != spex::IResourceContainer::kPortClosed) {
    return eStatusError;
  }
  return eStatusOk;
}


}}  // namespace autotests::client
