#include "ClientMessanger.h"

#include <Protocol.pb.h>
#include <Utils/StringUtils.h>

namespace autotests::client {

using String = utils::StringUtils;

MaybeError Messanger::openService(
    std::string_view sName,
    bool force,
    spex::IMessanger::Status* pStatus)
{
    {
        spex::Message message;
        auto* pBody = message.mutable_messanger()->mutable_open_service();
        pBody->set_service_name(std::string(sName));
        pBody->set_force(force);

        if (!send(std::move(message))) {
            return "Failed to send request";
        }
    }

    spex::IMessanger response;
    if (!wait(response)) {
        return "Timeout";
    }

    if (response.choice_case() != spex::IMessanger::kOpenServiceStatus) {
        return String::concat("Unexpected message: ", response.choice_case());
    }

    if (pStatus) {
        *pStatus = response.open_service_status();
    }

    if (response.open_service_status() != spex::IMessanger::SUCCESS) {
        return String::concat(
            "Non success status: ", response.open_service_status());
    }
    return std::nullopt;
}

MaybeError Messanger::getServicesList(std::vector<std::string>& services)
{
    {
        spex::Message message;
        message.mutable_messanger()->set_services_list_req(true);
        if (!send(std::move(message))) {
            return "Failed to send request";
        }
    }

    uint32_t nServicesLeft = 0;

    do {
        spex::IMessanger response;
        if (!wait(response)) {
            return "Response timeout";
        }

        if (response.choice_case() != spex::IMessanger::kServicesList) {
            return "Unexpected message";
        }

        const auto& servicesList = response.services_list();
        nServicesLeft = response.services_list().left();
        for (const std::string& service : servicesList.services()) {
            services.push_back(service);
        }
    } while (nServicesLeft);

    return std::nullopt;
}

MaybeError Messanger::sendRequest(
    std::string_view target,
    uint32_t         nSeq,
    std::string_view body,
    SessionStatus&   eSendStatus,
    uint32_t         nTimeoutMs)
{
    spex::Message message;
    spex::IMessanger::Request* req =
        message.mutable_messanger()->mutable_request();
    req->set_service(std::string(target));
    req->set_seq(nSeq);
    req->set_body(std::string(body));
    req->set_timeout_ms(nTimeoutMs);
    if (!send(std::move(message))) {
        return "Failed to send request";
    }

    MaybeError error = waitSessionStatus(eSendStatus);
    if (error) {
        return String::concat("No status response: ", *error);
    }
    // Even if status is not 'ROUTED', sendRequest() call should be treated
    // as succesfull
    return std::nullopt;
}

MaybeError Messanger::waitRequest(Request& request, uint32_t nTimeoutMs)
{
    spex::IMessanger message;
    if (!wait(message, nTimeoutMs)) {
        return "Response timeout";
    }

    if (message.choice_case() != spex::IMessanger::kRequest) {
        return "Unexpected message";
    }

    request.body = message.request().body();
    request.nSeq = message.request().seq();
    return std::nullopt;
}

MaybeError
Messanger::sendResponse(uint32_t nSeq, std::string_view body)
{
    spex::Message message;
    spex::IMessanger::Response* resp =
        message.mutable_messanger()->mutable_response();
    resp->set_seq(nSeq);
    resp->set_body(std::string(body));
    if (!send(std::move(message))) {
        return "Failed to send response";
    }
    return std::nullopt;
}

MaybeError Messanger::waitResponse(
    uint32_t& nSeq, std::string& body, uint32_t nTimeoutMs)
{
    spex::IMessanger message;
    if (!wait(message, nTimeoutMs)) {
        return "Response timeout";
    }

    if (message.choice_case() != spex::IMessanger::kResponse) {
        return String::concat("Unexpected message: ", message.DebugString());
    }

    nSeq = message.response().seq();
    body = message.response().body();

    return std::nullopt;
}

MaybeError Messanger::waitSessionStatus(SessionStatus& status)
{
    spex::IMessanger message;
    if (!wait(message)) {
        return "Response timeout";
    }

    if (message.choice_case() != spex::IMessanger::kSessionStatus) {
        return "Unexpected message";
    }

    status = {
        message.session_status().seq(),
        message.session_status().status()
    };
    return std::nullopt;
}

}  // namespace autotests::client
