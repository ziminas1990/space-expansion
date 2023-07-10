#pragma once

#include <memory>
#include <unordered_map>

#include <Newton/PhysicalObject.h>
#include <Modules/BaseModule.h>
#include <Utils/GlobalContainer.h>
#include <Utils/IndexedVector.hpp>
#include <Protocol.pb.h>

namespace modules
{

struct InternalData;

class Messanger : public BaseModule, public utils::GlobalObject<Messanger>
{
    struct Service {
        uint32_t    nSessionId = 0;
        std::string sName = std::string();
    };

    struct Transaction {
        uint32_t nServiceSessionId = 0;
        uint32_t nServiceSeq       = 0;
        uint32_t nClientSessionId  = 0;
        uint32_t nClientSeq        = 0;
        uint32_t nTimeoutMs        = 0;
        // Ingame timestamp specifies when transaction was created
        uint64_t nDeadline         = 0;
    };

    using Services = utils::IndexedVector<Service, decltype(Service::sName)>;
    using Transactions = utils::IndexedVector<
        Transaction, decltype(Transaction::nServiceSeq)>;


    Services     m_services;
    Transactions m_transactions;

    uint32_t     m_nNextSeq = 1;

public:
    Messanger(std::string&& sName, world::PlayerWeakPtr pOwner);

    void proceed(uint32_t nIntervalUs);

protected:
    // override from BaseModule
    void handleMessangerMessage(uint32_t nSessionId, spex::IMessanger const& message) override;

    void onSessionClosed(uint32_t nSessionId) override;

private:
    void openService(uint32_t nSessionId, const std::string& name, bool force);

    void onRequest(
        uint32_t           nSessionId,
        const std::string& sServiceName,
        uint32_t           nClientSeq,
        uint32_t           nTimeoutMs,
        const std::string& sBody);

    void onResponse(
        uint32_t           nSessionId,
        uint32_t           nServiceSeq,
        const std::string& sBody);

    void sendOpenServiceStatus(uint32_t nSessionId, spex::IMessanger::Status status);
    void sendServicesList(uint32_t nSessionId) const;
    void sendServiceInfo(uint32_t nSessionId, const Service& service, size_t left) const;
    void sendSessionStatus(
        uint32_t nSessionId, uint32_t nSeq, spex::IMessanger::Status status);
    void forwardRequest(const Transaction& transaction, const std::string& sBody);
    void forwardResponse(const Transaction& transaction, const std::string& sBody);

};

} // namespace modules
