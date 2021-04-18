#include "ClientBaseModule.h"

namespace autotests { namespace client {

static ClientBaseModule::MonitoringStatus convert(spex::IMonitor::Status eStatus) {
  switch (eStatus) {
    case spex::IMonitor::SUCCESS:
      return ClientBaseModule::eSuccess;
    case spex::IMonitor::LIMIT_EXCEEDED:
      return ClientBaseModule::eLimitExceeded;
    case spex::IMonitor::UNEXPECTED_MSG:
      return ClientBaseModule::eUnexpectedMessage;
    case spex::IMonitor::INVALID_TOKEN:
      return ClientBaseModule::eInvalidToken;
    default:
      assert(nullptr == "Unexpected status");
  }
}


ClientBaseModule::MonitoringStatus ClientBaseModule::subscribe(uint32_t &token)
{
  spex::Message request;
  request.mutable_monitor()->set_subscribe(true);
  if (!send(request)) {
    return MonitoringStatus::eTransportError;
  }

  spex::IMonitor response;
  if (!wait(response))
    return MonitoringStatus::eTimeout;
  if (response.choice_case() != spex::IMonitor::kToken)
    return MonitoringStatus::eUnexpectedMessage;

  token = response.token();
  return MonitoringStatus::eSuccess;
}

ClientBaseModule::MonitoringStatus ClientBaseModule::unsubscribe(uint32_t token)
{
  spex::Message request;
  request.mutable_monitor()->set_unsubscribe(token);
  if (!send(request)) {
    return MonitoringStatus::eTransportError;
  }

  spex::IMonitor response;
  if (!wait(response))
    return MonitoringStatus::eTimeout;
  if (response.choice_case() != spex::IMonitor::kStatus)
    return MonitoringStatus::eUnexpectedMessage;
  return convert(response.status());
}

}}
