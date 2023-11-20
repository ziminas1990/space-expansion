#include "Messanger.h"

#include <Utils/Clock.h>
#include <Modules/Constants.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::Messanger);

namespace modules {

Messanger::Messanger(std::string&& sName, world::PlayerWeakPtr pOwner)
    : BaseModule("Messanger", std::move(sName), pOwner)
    , m_services([](const Service& service) { return service.sName; })
    , m_transactions([](const Transaction& tr) { return tr.nServiceSeq; })
{
    registerSelf(this);
    // Always active
    switchToActiveState();
}

void Messanger::proceed(uint32_t)
{
    m_transactions.remove([this](const Transaction& transaction) {
        if (transaction.nDeadline <= utils::GlobalClock::now()) {
            sendSessionStatus(transaction.nClientSessionId,
                              transaction.nClientSeq,
                              spex::IMessanger::CLOSED);
            return true;
        }
        return false;
    });
}

void Messanger::handleMessangerMessage(
    uint32_t nSessionId, spex::IMessanger const& message)
{
    switch (message.choice_case())
    {
    case spex::IMessanger::kOpenService: {
        const spex::IMessanger::OpenService& req = message.open_service();
        openService(nSessionId, req.service_name(), req.force());
    } break;
    case spex::IMessanger::kServicesListReq: {
        sendServicesList(nSessionId);
    } break;
    case spex::IMessanger::kRequest: {
        const spex::IMessanger::Request& req = message.request();
        onRequest(
            nSessionId, req.service(), req.seq(), req.timeout_ms(), req.body());
    } break;
    case spex::IMessanger::kResponse: {
        const spex::IMessanger::Response& resp = message.response();
        onResponse(nSessionId, resp.seq(), resp.body());
    } break;
    default:
        break;
    }
}

void Messanger::onSessionClosed(uint32_t nSessionId)
{
    BaseModule::onSessionClosed(nSessionId);

    m_services.remove([nSessionId](const Service& service) {
        return service.nSessionId == nSessionId;
    });
}

void Messanger::openService(
                       uint32_t nSessionId, const std::string& name, bool force)
{
    if (m_services.has(name)) {
        if (force) {
            Service replaced;
            m_services.pop(name, replaced);
            closeSession(replaced.nSessionId);
        } else {
            sendOpenServiceStatus(nSessionId, spex::IMessanger::SERVICE_EXISTS);
            return;
        }
    }

    if (m_services.has([nSessionId](const Service& service) {
            return service.nSessionId == nSessionId;
        })) {
        sendOpenServiceStatus(nSessionId, spex::IMessanger::SESSION_BUSY);
        return;
    }

    if (m_services.size() >= constants::messanger::nServicesLimit) {
        sendOpenServiceStatus(nSessionId, spex::IMessanger::TOO_MANY_SERVCES);
        return;
    }

    m_services.push(Service{ nSessionId, name });
    sendOpenServiceStatus(nSessionId, spex::IMessanger::SUCCESS);
}

void Messanger::onRequest(
    uint32_t           nSessionId,
    const std::string& sServiceName,
    uint32_t           nClientSeq,
    uint32_t           nTimeoutMs,
    const std::string& sBody)
{
    const Service* pService = m_services.get(sServiceName);
    if (!pService) {
        sendSessionStatus(
                     nSessionId, nClientSeq, spex::IMessanger::NO_SUCH_SERVICE);
        return;
    }

    if (nTimeoutMs > constants::messanger::nMaxRequestTimeoutMs) {
        sendSessionStatus(
            nSessionId, nClientSeq, spex::IMessanger::REQUEST_TIMEOUT_TOO_LONG);
        return;
    }

    if (m_transactions.size() >= constants::messanger::nSessionsLimit) {
        sendSessionStatus(
            nSessionId, nClientSeq, spex::IMessanger::SESSIONS_LIMIT_REACHED);
        return;
    }

    const Transaction transaction{
        // Service sessionId & seq
        pService->nSessionId,
        m_nNextSeq++,
        // Client sessionId & seq
        nSessionId,
        nClientSeq,
        // Timeout
        nTimeoutMs * 1000,
        // Deadline
        utils::GlobalClock::now() + (nTimeoutMs * 1000)
    };

    if (forwardRequest(transaction, sBody)) {
        m_transactions.push(transaction);
        sendSessionStatus(
            nSessionId, nClientSeq, spex::IMessanger::ROUTED);
    } else {
        sendSessionStatus(
            nSessionId, nClientSeq, spex::IMessanger::UNKNOWN_ERROR);
    }
}

void Messanger::onResponse(uint32_t           nSessionId,
                           uint32_t           nServiceSeq,
                           const std::string& sBody)
{
    Transaction* pTransaction = m_transactions.get(nServiceSeq);
    if (pTransaction) {
        forwardResponse(*pTransaction, sBody);
        m_transactions.remove(pTransaction->nServiceSeq);
    } else {
        sendSessionStatus(nSessionId, nServiceSeq, spex::IMessanger::WRONG_SEQ);
    }
}

void
Messanger::sendOpenServiceStatus(
    uint32_t nSessionId, spex::IMessanger::Status status)
{
    spex::Message message;
    message.mutable_messanger()->set_open_service_status(status);
    sendToClient(nSessionId, std::move(message));
}

void Messanger::sendServicesList(uint32_t nSessionId) const
{
    size_t left = m_services.items().size();
    if (left == 0) {
        spex::Message message;
        spex::IMessanger::ServicesList* list =
            message.mutable_messanger()->mutable_services_list();
        list->set_left(0);
        sendToClient(nSessionId, std::move(message));
        return;
    }
    for (const Service& service : m_services.items()) {
        --left;
        sendServiceInfo(nSessionId, service, left);
    }
}

void Messanger::sendServiceInfo(
    uint32_t nSessionId, const Service& service, size_t left) const
{
    constexpr int nServicesInBatch = 32;

    static thread_local spex::Message message;
    spex::IMessanger::ServicesList* list =
        message.mutable_messanger()->mutable_services_list();
    list->add_services(service.sName);
    if (list->services_size() >= nServicesInBatch || left == 0) {
        list->set_left(left);
        sendToClient(nSessionId, std::move(message));
        message = spex::Message();
    }
}

void Messanger::sendSessionStatus(
    uint32_t nSessionId, uint32_t nSeq, spex::IMessanger::Status status)
{
    spex::Message message;
    spex::IMessanger::SessionStatus* sessionStatus =
        message.mutable_messanger()->mutable_session_status();
    sessionStatus->set_seq(nSeq);
    sessionStatus->set_status(status);
    sendToClient(nSessionId, std::move(message));
}

bool Messanger::forwardRequest(
    const Transaction& transaction, const std::string& sBody)
{
    spex::Message message;
    spex::IMessanger::Request* request =
                                 message.mutable_messanger()->mutable_request();

    request->set_seq(transaction.nServiceSeq);
    *request->mutable_body() = sBody;
    return sendToClient(transaction.nServiceSessionId, std::move(message));
}

void Messanger::forwardResponse(
    const Transaction& transaction, const std::string& sBody)
{
    spex::Message message;
    spex::IMessanger::Response* response =
                                message.mutable_messanger()->mutable_response();

    response->set_seq(transaction.nClientSeq);
    *response->mutable_body() = sBody;
    sendToClient(transaction.nClientSessionId, std::move(message));
}

} // namespace modules
