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


}}  // namespace autotests::client
