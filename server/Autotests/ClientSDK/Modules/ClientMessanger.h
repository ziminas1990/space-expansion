#pragma once

#include <stdint.h>
#include <string>
#include <string_view>
#include <vector>

#include <Autotests/ClientSDK/Interfaces.h>
#include <Autotests/ClientSDK/ClientBaseModule.h>
#include <Utils/UsefulTypes.hpp>

namespace autotests::client {

class Messanger : public ClientBaseModule
{
public:
    struct Request {
        uint32_t nSeq;
        std::string body;
    };

    struct SessionStatus {
        uint32_t                 nSeq = 0;
        spex::IMessanger::Status eStatus = spex::IMessanger::UNKNOWN_ERROR;

        bool is_ok() const {
            return eStatus == spex::IMessanger::SUCCESS
                || eStatus == spex::IMessanger::ROUTED;
        }

        bool is_routed() const {
            return eStatus == spex::IMessanger::ROUTED;
        }
    };

public:

    MaybeError openService(
        std::string_view sName,
        bool force,
        spex::IMessanger::Status* pStatus = nullptr);

    MaybeError getServicesList(std::vector<std::string>& services);

    MaybeError sendRequest(
        std::string_view target,
        uint32_t nSeq,
        std::string_view body,
        SessionStatus& eSendStatus,
        uint32_t nTimeoutMs = 1000);
    MaybeError waitRequest(Request& request, uint32_t nTimeoutMs = 100);

    MaybeError sendResponse(uint32_t nSeq, std::string_view body);
    MaybeError waitResponse(
        uint32_t& nSeq, std::string& body, uint32_t nTimeoutMs = 100);

    MaybeError waitSessionStatus(SessionStatus& status);

};

using MessangerPtr = std::shared_ptr<Messanger>;

}  // namespace autotests::client
