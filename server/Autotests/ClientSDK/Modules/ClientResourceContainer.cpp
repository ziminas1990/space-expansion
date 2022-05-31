#include "ClientResourceContainer.h"
#include <Utils/ItemsConverter.h>

namespace autotests { namespace client {

static ResourceContainer::Status convert(spex::IResourceContainer::Status status)
{
  switch (status) {
    case spex::IResourceContainer::SUCCESS:
      return ResourceContainer::eStatusOk;
    case spex::IResourceContainer::PORT_ALREADY_OPEN:
      return ResourceContainer::ePortAlreadyOpen;
    case spex::IResourceContainer::PORT_DOESNT_EXIST:
    case spex::IResourceContainer::PORT_IS_NOT_OPENED:
      return ResourceContainer::ePortIsNotOpened;
    case spex::IResourceContainer::PORT_HAS_BEEN_CLOSED:
      return ResourceContainer::ePortHasBeenClosed;
    case spex::IResourceContainer::INVALID_ACCESS_KEY:
      return ResourceContainer::eInvalidAccessKey;
    case spex::IResourceContainer::INVALID_RESOURCE_TYPE:
      return ResourceContainer::eInvalidResource;
    case spex::IResourceContainer::PORT_TOO_FAR:
      return ResourceContainer::eTransferTooFar;
    case spex::IResourceContainer::TRANSFER_IN_PROGRESS:
      return ResourceContainer::eTransferInProgress;
    case spex::IResourceContainer::NOT_ENOUGH_RESOURCES:
      return ResourceContainer::eNotEnoughResources;
    case spex::IResourceContainer::INTERNAL_ERROR:
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
    world::Resource::Type eType = utils::convert(item.type());
    content.m_amount[eType] = item.amount();
  }
  return true;
}

ResourceContainer::Content& ResourceContainer::Content::set(
    world::Resource::Type eType, double amount)
{
  assert(eType < m_amount.size());
  m_nUsedSpace    -= m_amount[eType] / world::Resource::Density[eType];
  m_amount[eType]  = amount;
  m_nUsedSpace    += amount / world::Resource::Density[eType];
  return *this;
}

bool ResourceContainer::getContent(ResourceContainer::Content &content)
{
  spex::Message request;
  request.mutable_resource_container()->set_content_req(true);
  return send(std::move(request)) && waitContent(content);
}

ResourceContainer::Status ResourceContainer::openPort(
    uint32_t nAccessKey, uint32_t& nPortId)
{
  spex::Message request;
  request.mutable_resource_container()->set_open_port(nAccessKey);

  if (!send(std::move(request)))
    return eStatusError;

  spex::IResourceContainer response;
  if (!wait(response))
    return eStatusError;

  if (response.choice_case() == spex::IResourceContainer::kOpenPortFailed) {
    return convert(response.open_port_failed());
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

  if (!send(std::move(request)))
    return eStatusError;

  spex::IResourceContainer response;
  if (!wait(response))
    return eStatusError;
  if (response.choice_case() != spex::IResourceContainer::kClosePortStatus) {
    return eStatusError;
  }
  return convert(response.close_port_status());
}

ResourceContainer::Status ResourceContainer::transferRequest(
    uint32_t nPortId, uint32_t nAccessKey, world::Resource::Type type, double amount)
{
  spex::Message message;
  spex::IResourceContainer::Transfer* pRequest =
      message.mutable_resource_container()->mutable_transfer();

  pRequest->set_port_id(nPortId);
  pRequest->set_access_key(nAccessKey);
  pRequest->mutable_resource()->set_type(utils::convert(type));
  pRequest->mutable_resource()->set_amount(amount);

  if (!send(std::move(message)))
    return eStatusError;

  spex::IResourceContainer response;
  if (!wait(response)) {
    return eStatusError;
  }

  if (response.choice_case() != spex::IResourceContainer::kTransferStatus) {
    return eStatusError;
  }

  return convert(response.transfer_status());
}

bool ResourceContainer::monitor(Content &content)
{
  spex::Message request;
  request.mutable_resource_container()->set_monitor(true);
  return send(std::move(request)) && waitContent(content);
}

ResourceContainer::Status
ResourceContainer::waitTransferReport(double& amount)
{
  spex::IResourceContainer response;
  if (!wait(response)) {
    return eStatusError;
  }

  switch (response.choice_case()) {
    case spex::IResourceContainer::kTransferReport: {
      amount += response.transfer_report().amount();
      return ResourceContainer::eTransferInProgress;
    }
    case spex::IResourceContainer::kTransferFinished: {
      return convert(response.transfer_finished());
    }
    default:
      // Got unexpected response
      assert(false);
      return eStatusError;
  }
}

ResourceContainer::Status ResourceContainer::waitTransfer(double amount)
{
  client::ResourceContainer::Status status =
      client::ResourceContainer::eTransferInProgress;
  double totalTransferred = 0;
  while (status == client::ResourceContainer::eTransferInProgress) {
    status = waitTransferReport(totalTransferred);
  }
  if (status != client::ResourceContainer::eStatusOk) {
    return status;
  }
  if (abs(amount - totalTransferred) > 0.001) {
    return eTransferIncomplete;
  }
  return status;
}

bool ResourceContainer::waitContent(Content &content)
{
  spex::IResourceContainer response;
  if (!wait(response))
    return false;
  if (response.choice_case() != spex::IResourceContainer::kContent)
    return false;
  return fillContent(content, response.content());
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

  for (world::Resource::Type eType : world::Resource::MaterialResources)
    if (std::abs(expected.m_amount[eType] - content.m_amount[eType]) > 0.01)
      return false;

  return true;
}

bool ResourceContainer::checkContent(world::ResourcesArray const& expected, double delta)
{
  ResourceContainer::Content content;
  if (!getContent(content))
    return false;

  for (size_t i = 0; i < expected.size(); ++i)
    if (std::abs(expected[i] - content.m_amount[i]) > delta)
      return false;

  return true;
}

}}  // namespace autotests::client
