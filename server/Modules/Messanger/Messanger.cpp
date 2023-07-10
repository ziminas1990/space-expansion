#include "Messanger.h"
#include <Utils/Clock.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::Messanger);

namespace modules {

void Messanger::proceed(uint32_t)
{
    m_transactions.remove([this](const Transaction& transaction) {
        if (transaction.nDeadline > utils::GlobalClock::now()) {
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
        onRequest(nSessionId, req.service(), req.seq(), req.timeout_ms(), req.body());
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
    m_services.remove([nSessionId](const Service& service) {
        return service.nSessionId == nSessionId;
        });
}

void Messanger::openService(uint32_t nSessionId, const std::string& name, bool force)
{
    constexpr size_t nServicesLimit = 32;

    if (m_services.has(name)) {
        if (force) {
            Service replaced;
            m_services.pop(name, replaced);
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

    if (m_services.size() > nServicesLimit) {
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
        sendSessionStatus(nSessionId, nClientSeq, spex::IMessanger::NO_SUCH_SERVICE);
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

    m_transactions.push(transaction);
    forwardRequest(transaction, sBody);
}

void Messanger::onResponse(
    uint32_t           nSessionId,
    uint32_t           nServiceSeq,
    const std::string& sBody)
{
    Transaction* pTransaction = m_transactions.get(nServiceSeq);
    if (!pTransaction) {
        sendSessionStatus(nSessionId, nServiceSeq, spex::IMessanger::WRONG_SEQ);
        return;
    }

    // Each response reset session timeout
    pTransaction->nDeadline = utils::GlobalClock::now() + (pTransaction->nTimeoutMs * 1000);

    forwardResponse(*pTransaction, sBody);
}

void
Messanger::sendOpenServiceStatus(uint32_t nSessionId, spex::IMessanger::Status status)
{
    spex::Message message;
    message.mutable_messanger()->set_open_service_status(status);
    sendToClient(nSessionId, std::move(message));
}

void Messanger::sendServicesList(uint32_t nSessionId) const
{
    size_t left = m_services.items().size();
    for (const Service& service : m_services.items()) {
        --left;
        sendServiceInfo(nSessionId, service, left);
    }
}

void Messanger::sendServiceInfo(uint32_t nSessionId, const Service& service, size_t left) const
{
    constexpr int nServicesInBatch = 32;

    static thread_local spex::Message message;
    spex::IMessanger::ServicesList* list =
        message.mutable_messanger()->mutable_services_list();
    list->add_services(service.sName);
    if (list->services_size() >= nServicesInBatch || left == 0) {
        list->set_left(left);
        sendToClient(nSessionId, std::move(message));
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

void Messanger::forwardRequest(const Transaction& transaction, const std::string& sBody)
{
    spex::Message message;
    spex::IMessanger::Request* request = message.mutable_messanger()->mutable_request();

    request->set_seq(transaction.nServiceSeq);
    *request->mutable_body() = sBody;
    sendToClient(transaction.nServiceSessionId, std::move(message));
}

void Messanger::forwardResponse(const Transaction& transaction, const std::string& sBody)
{
    spex::Message message;
    spex::IMessanger::Response* response = message.mutable_messanger()->mutable_response();

    response->set_seq(transaction.nClientSeq);
    *response->mutable_body() = sBody;
    sendToClient(transaction.nClientSessionId, std::move(message));
}

} // namespace modules
