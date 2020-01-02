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

static spex::ResourceType convert(world::Resources::Type type)
{
  switch (type) {
    case world::Resources::eMettal:
      return spex::ResourceType::RESOURCE_METTALS;
    case world::Resources::eSilicate:
      return spex::ResourceType::RESOURCE_SILICATES;
    case world::Resources::eIce:
      return spex::ResourceType::RESOURCE_ICE;
    default:
      assert(false);
      return spex::ResourceType::RESOURCE_METTALS;  // ?
  }
}

static ResourceContainer::Status convert(spex::IResourceContainer::Error error)
{
  switch (error) {
    case spex::IResourceContainer::ERROR_PORT_ALREADY_OPEN:
      return ResourceContainer::ePortAlreadyOpen;
    case spex::IResourceContainer::ERROR_PORT_DOESNT_EXIST:
    case spex::IResourceContainer::ERROR_PORT_IS_NOT_OPENED:
      return ResourceContainer::ePortIsNotOpened;
    case spex::IResourceContainer::ERROR_PORT_HAS_BEEN_CLOSED:
      return ResourceContainer::ePortHasBeenClosed;
    case spex::IResourceContainer::ERROR_INVALID_KEY:
      return ResourceContainer::eInvalidAccessKey;
    case spex::IResourceContainer::ERROR_TOO_FAR:
      return ResourceContainer::eTransferTooFar;
    case spex::IResourceContainer::ERROR_TRANSFER_IN_PROGRESS:
      return ResourceContainer::eTransferInProgress;
    case spex::IResourceContainer::ERROR_NOT_ENOUGH_RESOURCES:
      return ResourceContainer::eNotEnoughResources;
    case spex::IResourceContainer::ERROR_INTERNAL:
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

ResourceContainer::Content& ResourceContainer::Content::set(
    world::Resources::Type eType, double amount)
{
  assert(eType < m_amount.size());
  m_nUsedSpace    -= m_amount[eType] / world::Resources::density[eType];
  m_amount[eType]  = amount;
  m_nUsedSpace    += amount / world::Resources::density[eType];
  return *this;
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

ResourceContainer::Status ResourceContainer::transfer(
    uint32_t nPortId, uint32_t nAccessKey, world::Resources::Type type, double amount)
{
  spex::Message message;
  spex::IResourceContainer::Transfer* pRequest =
      message.mutable_resource_container()->mutable_transfer();

  pRequest->set_port_id(nPortId);
  pRequest->set_access_key(nAccessKey);
  pRequest->mutable_resource()->set_type(convert(type));
  pRequest->mutable_resource()->set_amount(amount);

  if (!send(message))
    return eStatusError;

  while (true) {
    spex::IResourceContainer response;
    if (!wait(response)) {
      return eStatusError;
    }

    switch (response.choice_case()) {
      case spex::IResourceContainer::kOnError: {
        return convert(response.on_error());
      }
      case spex::IResourceContainer::kTransferReport: {
        if (convert(response.transfer_report().type()) != type) {
          return eTransferGotInvalidReport;
        }
        amount -= response.transfer_report().amount();
        break;
      }
      case spex::IResourceContainer::kTransferFailed: {
        return convert(response.transfer_failed());
      }
      case spex::IResourceContainer::kTransferComplete: {
        if (std::abs(amount) > 0.001) {
          return eTransferIncomplete;
        }
        return eStatusOk;
      }
      default:
        return eStatusError;
    }
  }
}

bool ResourceContainer::checkContent(ResourceContainer::Content const& expected)
{
  ResourceContainer::Content content;
  if (!getContent(content))
    return false;

  if (content.m_amount.size() != expected.m_amount.size())
    return false;

  if (std::abs(content.m_nUsedSpace - expected.m_nUsedSpace) > 0.01)
    return false;

  for (size_t i = 0; i < expected.m_amount.size(); ++i)
    if (std::abs(expected.m_amount[i] - content.m_amount[i]) > 0.01)
      return false;

  return true;
}

}}  // namespace autotests::client