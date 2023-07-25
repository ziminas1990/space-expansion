#include <gtest/gtest.h>

#include <Modules/Messanger/Messanger.h>
#include <Autotests/ClientSDK/Modules/ClientMessanger.h>
#include <Utils/UsefulTypes.hpp>
#include <Utils/StringUtils.h>
#include <Modules/Constants.h>

#include "ModulesTestFixture.h"
#include "Helper.h"

namespace autotests {

using String = utils::StringUtils;

class MessangerTests : public ModulesTestFixture
{
public:
    struct Service {
        std::string              sName;
        client::MessangerPtr     pSession;
        MaybeError               sProblem;
        spex::IMessanger::Status eOpenStatus;

        Service(std::string_view sName, client::MessangerPtr pSession)
            : sName(sName), pSession(pSession), eOpenStatus(spex::IMessanger::SUCCESS)
        {}

        Service(std::string_view sName,
                std::string_view sProblem,
                spex::IMessanger::Status status = spex::IMessanger::UNKNOWN_ERROR)
            : sName(sName), sProblem(sProblem), eOpenStatus(status)
        {}

        void close() { pSession->disconnect(); }
        operator bool() const { return !!pSession; }
        client::Messanger& operator->() { return *pSession; }
        std::string problem() { return sProblem ? *sProblem : std::string(); }
    };


    Service spawnService(
        client::ClientCommutatorPtr pCommutator,
        std::string_view name)
    {
        client::MessangerPtr pSession = Helper::getMessanger(pCommutator);
        if (!pSession) {
            return Service(name, String::concat("can't open messanger session"));
        }

        spex::IMessanger::Status status;
        MaybeError problem = pSession->openService(name, false, &status);
        if (problem) {
            pSession->disconnect();
            const std::string error = String::concat(
                "can't open service ", name, ": ", *problem);
            return Service(name, std::move(error), status);
        }

        return Service(std::string(name), pSession);
    }

    MaybeError reopenService(
        client::ClientCommutatorPtr pCommutator,
        Service& service)
    {
        client::MessangerPtr pSession = Helper::getMessanger(pCommutator);
        if (!pSession) {
            return String::concat("can't open messanger session");
        }

        spex::IMessanger::Status status;
        MaybeError problem = pSession->openService(service.sName, true, &status);
        if (problem) {
            pSession->disconnect();
            return String::concat("can't open service ", service.sName, ": ", *problem);
        }

        if (!service.pSession->waitCloseInd()) {
            pSession->disconnect();
            return "old session wasn't closed";
        }

        service.pSession = pSession;
        return std::nullopt;
    }

    MaybeError checkServiceList(
        client::MessangerPtr pMessagner,
        std::vector<Service> expected)
    {
        std::vector<std::string> services;
        MaybeError error = pMessagner->getServicesList(services);
        if (error) {
            return String::concat("Can't get services list: ", *error);
        }

        if (services.size() != expected.size()) {
            return String::concat("Got ", services.size(), " services, but expected ",
                                  expected.size());
        }

        std::set<std::string> services_set;
        for (std::string name : services) {
            services_set.emplace(std::move(name));
        }

        std::vector<std::string> problems;

        for (const Service& expected_service : expected) {
            auto I = services_set.find(expected_service.sName);
            if (I == services_set.end()) {
                problems.push_back(
                    String::concat("Service is not listed: ", expected_service.sName));
            } else {
                services_set.erase(I);
            }
        }

        // 'services_set' should be empty now
        for (const std::string& unexpected_service : services_set) {
            problems.push_back(
                String::concat("Got unexpected service: ", unexpected_service));
        }

        return problems.empty() ? MaybeError() : String::join(problems, " AND ");
    }

    MaybeError pingService(client::MessangerPtr pMessagner, const Service& service)
    {
        static uint32_t nNextSeq = 0;

        const uint32_t    nSeq              = nNextSeq++;
        const std::string request           = String::concat("Ping req #", nSeq);
        const std::string expected_response = String::concat("Ping resp #", nSeq);

        // Send request
        {
            MaybeError error = pMessagner->sendRequest(service.sName, nSeq, request);
            if (error) {
                return String::concat("Can't send request: ", *error);
            }
        }

        // Handle request on service size:
        {
            client::Messanger::Request receivedRequest;
            MaybeError error = service.pSession->waitRequest(receivedRequest);
            if (error) {
                return String::concat("Service haven't got a request: ", *error);
            }
            if (receivedRequest.body != request) {
                return String::concat(
                    "Service got unexpected request: '", receivedRequest.body,
                    "', expected: ", expected_response);
            }
            error = service.pSession->sendResponse(receivedRequest.nSeq,
                                                   expected_response);
            if (error) {
                return String::concat("Can't send response: ", *error);
            }
        }

        // Receive response on client side:
        {
            uint32_t nResponseSeq = 0;
            std::string sResponseBody;
            MaybeError error = pMessagner->waitResponse(nResponseSeq, sResponseBody);
            if (error) {
                return String::concat("Failed to receive response: ", *error);
            }
            if (nResponseSeq != nSeq) {
                return String::concat(
                    "Client got response with unexpected seq: ", nResponseSeq,
                    ", expected: ", nSeq);
            }
            if (sResponseBody != expected_response) {
                return String::concat(
                    "Client got unexpected response: '", sResponseBody,
                    "', expected: ", expected_response);
            }
        }
        return std::nullopt;
    }

    MaybeError pingService(client::ClientCommutatorPtr pCommutator, const Service& service) {
        client::MessangerPtr pMessanger = Helper::getMessanger(pCommutator);
        if (!pMessanger) {
            return "can't open session";
        }
        MaybeError error = pingService(pMessanger, service);
        pMessanger->disconnect();
        return error;
    }

    MaybeError pingServices(
        client::ClientCommutatorPtr pCommutator,
        const std::vector<Service>& services,
        uint32_t nClinets, size_t repeats)
    {
        std::vector<client::MessangerPtr> clients;
        for (uint32_t i = 1; i <= nClinets; ++i) {
            client::MessangerPtr pMessanger = Helper::getMessanger(pCommutator);
            if (!pMessanger) {
                return String::concat("can't open session for client #", i);
            }
            clients.push_back(pMessanger);
        }

        const auto disconnectClients = [&clients]() {
            for (client::MessangerPtr pClient : clients) {
                pClient->disconnect();
            }
        };

        uint32_t nNextClientId = 0;

        for (size_t i = 0; i < repeats; ++i) {
            for (const Service& service : services) {
                client::MessangerPtr pClient = clients[nNextClientId++ % clients.size()];
                const MaybeError error = pingService(pClient, service);
                if (error) {
                    disconnectClients();
                    return String::concat(
                        "Failed to ping service ", service.sName, " on repeat #", i,
                        ": ", *error);
                }
            }
        }

        disconnectClients();
        return std::nullopt;
    }

    MaybeError echoAllRequests(const Service& service, size_t total)
    {
        for (size_t i = 0; i < total; ++i){
            client::Messanger::Request request;
            MaybeError error = service.pSession->waitRequest(request);
            if (error) {
                return String::concat("can't get request: ", *error);
            }

            error = service.pSession->sendResponse(request.nSeq, request.body);
            if (error) {
                return String::concat("can't echo request: ", *error);
            }
        }
        return std::nullopt;
    }
};

TEST_F(MessangerTests, OpenService)
{
    Helper::createMessangerModule(*this);

    client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
    ASSERT_TRUE(pRootSession);
    client::ClientCommutatorPtr pCommutator =
        Helper::openCommutatorSession(*this, pRootSession);
    ASSERT_TRUE(pCommutator);

    Service service = spawnService(pCommutator, "SomeService");
    ASSERT_TRUE(service) << "can't spawn service: " << service.problem();
}

TEST_F(MessangerTests, ServiceExists)
{
    Helper::createMessangerModule(*this);

    client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
    ASSERT_TRUE(pRootSession);
    client::ClientCommutatorPtr pCommutator =
        Helper::openCommutatorSession(*this, pRootSession);
    ASSERT_TRUE(pCommutator);

    Service service = spawnService(pCommutator, "SomeService");
    ASSERT_TRUE(service) << "can't spawn service: " << service.problem();

    {
        client::MessangerPtr pMessanger = Helper::getMessanger(pCommutator);
        ASSERT_TRUE(pMessanger);

        spex::IMessanger::Status status;
        MaybeError error = service.pSession->openService("SomeService", false, &status);

        ASSERT_EQ(spex::IMessanger::SERVICE_EXISTS, status);
        ASSERT_TRUE(error);
    }
}

TEST_F(MessangerTests, SessionBusy)
{
    Helper::createMessangerModule(*this);

    client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
    ASSERT_TRUE(pRootSession);
    client::ClientCommutatorPtr pCommutator =
        Helper::openCommutatorSession(*this, pRootSession);
    ASSERT_TRUE(pCommutator);

    Service service = spawnService(pCommutator, "SomeService");
    ASSERT_TRUE(service) << "can't spawn service: " << service.problem();

    // Attempt to create the same service should fail
    {
        spex::IMessanger::Status status;
        MaybeError error = service.pSession->openService("SecondService", false, &status);

        ASSERT_EQ(spex::IMessanger::SESSION_BUSY, status);
        ASSERT_TRUE(error);
    }
}

TEST_F(MessangerTests, TwoManyServices)
{
    constexpr size_t nExpectedLimit = modules::constants::messanger::nServicesLimit;

    Helper::createMessangerModule(*this);

    client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
    ASSERT_TRUE(pRootSession);
    client::ClientCommutatorPtr pCommutator =
        Helper::openCommutatorSession(*this, pRootSession);
    ASSERT_TRUE(pCommutator);

    client::MessangerPtr pMessanger = Helper::getMessanger(pCommutator);
    ASSERT_TRUE(pMessanger);

    std::vector<Service> services;

    for (size_t i = 1; i <= nExpectedLimit; ++i) {
        const std::string name = String::concat("SomeService_", i);
        Service service = spawnService(pCommutator, name);
        ASSERT_TRUE(service) << "Can't spawn service: " << service.problem();
        services.emplace_back(std::move(service));
    }

    for (size_t i = nExpectedLimit + 1; i < 2 * nExpectedLimit; ++i) {
        const std::string name = String::concat("SomeService_", i);
        Service service = spawnService(pCommutator, name);
        ASSERT_FALSE(service);
        ASSERT_EQ(spex::IMessanger::TOO_MANY_SERVCES, service.eOpenStatus);
    }
}

TEST_F(MessangerTests, ServicesList)
{
    constexpr size_t nExpectedLimit = modules::constants::messanger::nServicesLimit;

    Helper::createMessangerModule(*this);

    client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
    ASSERT_TRUE(pRootSession);
    client::ClientCommutatorPtr pCommutator =
        Helper::openCommutatorSession(*this, pRootSession);
    ASSERT_TRUE(pCommutator);

    // Will always contain currently opened services (can be used as expected
    // list of services)
    std::vector<Service> services;

    uint32_t nNextServiceId = 1;
    const auto generateServiceName = [&nNextServiceId]() {
        return String::concat("SomeService_", nNextServiceId++);
    };

    for (size_t i = 1; i <= nExpectedLimit; ++i) {
        Service service = spawnService(pCommutator, generateServiceName());
        ASSERT_TRUE(service) << "Can't spawn service: " << service.problem();
        services.emplace_back(std::move(service));
    }

    client::MessangerPtr pMessanger = Helper::getMessanger(pCommutator);
    ASSERT_TRUE(pMessanger);

    // Check services list
    auto rng = std::default_random_engine{};
    uint8_t action = 0;
    while (!services.empty()) {
        std::shuffle(services.begin(), services.end(), rng);

        MaybeError error = checkServiceList(pMessanger, services);
        ASSERT_FALSE(error) << *error;

        // remove 2 services, add 1 service
        ++action;
        if (action < 3) {
            Service service = services.back();
            services.pop_back();
            service.close();
        } else {
            action = 0;
            Service service = spawnService(pCommutator, generateServiceName());
            ASSERT_TRUE(service) << "Can't spawn service: " << service.problem();
            services.emplace_back(std::move(service));
        }
    }

}

TEST_F(MessangerTests, ReopenService)
{
    constexpr size_t nExpectedLimit = modules::constants::messanger::nServicesLimit;

    Helper::createMessangerModule(*this);

    client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
    ASSERT_TRUE(pRootSession);
    client::ClientCommutatorPtr pCommutator =
        Helper::openCommutatorSession(*this, pRootSession);
    ASSERT_TRUE(pCommutator);

    client::MessangerPtr pMessanger = Helper::getMessanger(pCommutator);
    ASSERT_TRUE(pMessanger);

    std::vector<Service> services;

    for (size_t i = 1; i <= nExpectedLimit; ++i) {
        const std::string name = String::concat("SomeService_", i);
        Service service = spawnService(pCommutator, name);
        ASSERT_TRUE(service) << "Can't spawn service: " << service.problem();
        services.emplace_back(std::move(service));
    }

    {
        MaybeError error = checkServiceList(pMessanger, services);
        ASSERT_FALSE(error) << "Services list check failed: " << *error;
        error = pingServices(pCommutator, services, 7, 10);
        ASSERT_FALSE(error) << "Ping check failed: " << *error;
    }

    // Reopen each service 16 times in random order
    auto rng = std::default_random_engine{};
    for (size_t i = 0; i < 16; ++i) {
        std::shuffle(services.begin(), services.end(), rng);
        for (Service& service : services) {
            const MaybeError error = reopenService(pCommutator, service);
            ASSERT_FALSE(error) << "can't reopen service " << service.sName
                << ": " << *error << " (i = " << i << ")";
        }

        {
            MaybeError error = checkServiceList(pMessanger, services);
            ASSERT_FALSE(error) << "Services list check failed: " << *error;
            error = pingServices(pCommutator, services, 7, 10);
            ASSERT_FALSE(error) << "Ping check failed: " << *error;
        }
    }
}

TEST_F(MessangerTests, SendRequestToWrongService)
{
    Helper::createMessangerModule(*this);

    client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
    ASSERT_TRUE(pRootSession);
    client::ClientCommutatorPtr pCommutator =
        Helper::openCommutatorSession(*this, pRootSession);
    ASSERT_TRUE(pCommutator);

    client::MessangerPtr pMessanger = Helper::getMessanger(pCommutator);
    ASSERT_TRUE(pMessanger);

    constexpr uint32_t nSomeSeq = 12345;

    MaybeError error = pMessanger->sendRequest(
        "NonExistingService", nSomeSeq, "Ping");
    ASSERT_FALSE(error) << "Can't sent request: " << *error;

    client::Messanger::SessionStatus status;
    error = pMessanger->waitSessionStatus(status);
    ASSERT_FALSE(error) << "Can't get session status: " << *error;

    ASSERT_EQ(nSomeSeq, status.nSeq);
    ASSERT_EQ(spex::IMessanger::NO_SUCH_SERVICE, status.eStatus);
}

TEST_F(MessangerTests, SendRequestWithWrongTimeout)
{
    Helper::createMessangerModule(*this);

    client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
    ASSERT_TRUE(pRootSession);
    client::ClientCommutatorPtr pCommutator =
        Helper::openCommutatorSession(*this, pRootSession);
    ASSERT_TRUE(pCommutator);

    const char* sServiceName = "SomeAwesomeService";
    Service service = spawnService(pCommutator, sServiceName);

    client::MessangerPtr pMessanger = Helper::getMessanger(pCommutator);
    ASSERT_TRUE(pMessanger);

    constexpr uint32_t nSomeSeq = 12345;

    MaybeError error = pMessanger->sendRequest(
        sServiceName, nSomeSeq, "Ping",
        modules::constants::messanger::nMaxRequestTimeoutMs + 1);
    ASSERT_FALSE(error) << "Can't sent request: " << *error;

    client::Messanger::SessionStatus status;
    error = pMessanger->waitSessionStatus(status);
    ASSERT_FALSE(error) << "Can't get session status: " << *error;

    ASSERT_EQ(nSomeSeq, status.nSeq);
    ASSERT_EQ(spex::IMessanger::REQUEST_TIMEOUT_TOO_LONG, status.eStatus);
}

TEST_F(MessangerTests, TooManySessions)
{
    constexpr size_t nSessionsLimit = modules::constants::messanger::nSessionsLimit;

    Helper::createMessangerModule(*this);

    client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
    ASSERT_TRUE(pRootSession);
    client::ClientCommutatorPtr pCommutator =
        Helper::openCommutatorSession(*this, pRootSession);
    ASSERT_TRUE(pCommutator);

    const char* sServiceName = "SomeAwesomeService";
    Service service = spawnService(pCommutator, sServiceName);

    client::MessangerPtr pMessanger = Helper::getMessanger(pCommutator);
    ASSERT_TRUE(pMessanger);

    // Fill the sessions limit:
    for (size_t seq = 0; seq < nSessionsLimit; ++seq) {
        MaybeError error = pMessanger->sendRequest(
            sServiceName, seq, "Ping",
            modules::constants::messanger::nMaxRequestTimeoutMs);
        ASSERT_FALSE(error) << "Can't sent request: " << *error;
    }

    // Send one more request (sessions limit will be exceeded)
    const size_t nLastSeq = nSessionsLimit;
    MaybeError error = pMessanger->sendRequest(
        sServiceName, nLastSeq, "Ping",
        modules::constants::messanger::nMaxRequestTimeoutMs);
    ASSERT_FALSE(error) << "Can't sent request: " << *error;

    client::Messanger::SessionStatus status;
    error = pMessanger->waitSessionStatus(status);
    ASSERT_FALSE(error) << "Can't get session status: " << *error;

    ASSERT_EQ(nLastSeq, status.nSeq);
    ASSERT_EQ(spex::IMessanger::SESSIONS_LIMIT_REACHED, status.eStatus);
}

TEST_F(MessangerTests, TwoSessionsUsesTheSameSeq)
{
    Helper::createMessangerModule(*this);

    client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
    ASSERT_TRUE(pRootSession);
    client::ClientCommutatorPtr pCommutator =
        Helper::openCommutatorSession(*this, pRootSession);
    ASSERT_TRUE(pCommutator);

    client::MessangerPtr pClientA = Helper::getMessanger(pCommutator);
    ASSERT_TRUE(pClientA);
    client::MessangerPtr pClientB = Helper::getMessanger(pCommutator);
    ASSERT_TRUE(pClientB);

    const char* sServiceName = "SomeAwesomeService";
    Service service = spawnService(pCommutator, sServiceName);

    constexpr uint32_t nSomeSeq = 3324;

    // Two clients send request with the same 'seq' to the same service
    MaybeError error;
    error = pClientA->sendRequest(sServiceName, nSomeSeq, "AAA");
    ASSERT_FALSE(error) << "Can't sent request: " << *error;
    error = pClientB->sendRequest(sServiceName, nSomeSeq, "BBB");
    ASSERT_FALSE(error) << "Can't sent request: " << *error;

    error = echoAllRequests(service, 2);
    ASSERT_FALSE(error) << *error;

    // Check that both clients will get an answer
    {
        uint32_t    nResponseSeq;
        std::string nResponseBody;
        error = pClientA->waitResponse(nResponseSeq, nResponseBody);
        ASSERT_FALSE(error) << *error;
        ASSERT_EQ(nSomeSeq, nResponseSeq);
        ASSERT_EQ("AAA", nResponseBody);
    }
    {
        uint32_t    nResponseSeq;
        std::string nResponseBody;
        error = pClientB->waitResponse(nResponseSeq, nResponseBody);
        ASSERT_FALSE(error) << *error;
        ASSERT_EQ(nSomeSeq, nResponseSeq);
        ASSERT_EQ("BBB", nResponseBody);
    }
}

TEST_F(MessangerTests, SendTwoFinalResponses)
{
    Helper::createMessangerModule(*this);

    client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
    ASSERT_TRUE(pRootSession);
    client::ClientCommutatorPtr pCommutator =
        Helper::openCommutatorSession(*this, pRootSession);
    ASSERT_TRUE(pCommutator);

    client::MessangerPtr pClient = Helper::getMessanger(pCommutator);
    ASSERT_TRUE(pClient);

    const char* sServiceName = "SomeBadService";
    Service service = spawnService(pCommutator, sServiceName);

    constexpr uint32_t nSomeSeq = 3324;

    MaybeError error;
    error = pClient->sendRequest(sServiceName, nSomeSeq, "AAA");
    ASSERT_FALSE(error) << "Can't sent request: " << *error;

    client::Messanger::Request request;
    error = service.pSession->waitRequest(request);
    ASSERT_FALSE(error) << "Can't get request: " << *error;

    error = service.pSession->sendResponse(request.nSeq, request.body);
    ASSERT_FALSE(error) << "Can't get first response: " << *error;
    error = service.pSession->sendResponse(request.nSeq, request.body);
    ASSERT_FALSE(error) << "Can't get second response: " << *error;

    client::Messanger::SessionStatus status;
    error = service.pSession->waitSessionStatus(status);
    ASSERT_FALSE(error) << "Can't get session status: " << *error;
    ASSERT_EQ(request.nSeq, status.nSeq);
    ASSERT_EQ(spex::IMessanger::WRONG_SEQ, status.eStatus);
}

TEST_F(MessangerTests, SendResponseWithWrongSeq)
{
    Helper::createMessangerModule(*this);

    client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
    ASSERT_TRUE(pRootSession);
    client::ClientCommutatorPtr pCommutator =
        Helper::openCommutatorSession(*this, pRootSession);
    ASSERT_TRUE(pCommutator);

    client::MessangerPtr pClient = Helper::getMessanger(pCommutator);
    ASSERT_TRUE(pClient);

    const char* sServiceName = "SomeBadService";
    Service service = spawnService(pCommutator, sServiceName);

    constexpr uint32_t nSomeSeq = 3324;

    MaybeError error;
    error = pClient->sendRequest(sServiceName, nSomeSeq, "PING REQ");
    ASSERT_FALSE(error) << "Can't sent request: " << *error;

    client::Messanger::Request request;
    error = service.pSession->waitRequest(request);
    ASSERT_FALSE(error) << "Can't get request: " << *error;

    const uint32_t nWrongSeq = request.nSeq + 1;
    error = service.pSession->sendResponse(nWrongSeq, request.body);
    ASSERT_FALSE(error) << "Can't second response: " << *error;

    client::Messanger::SessionStatus status;
    error = service.pSession->waitSessionStatus(status);
    ASSERT_FALSE(error) << "Can't get session status: " << *error;
    ASSERT_EQ(nWrongSeq, status.nSeq);
    ASSERT_EQ(spex::IMessanger::WRONG_SEQ, status.eStatus);
}

TEST_F(MessangerTests, LongSession)
{
    // Check that session will be active while non-final responses are being
    // sent to client

    Helper::createMessangerModule(*this);

    client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
    ASSERT_TRUE(pRootSession);
    client::ClientCommutatorPtr pCommutator =
        Helper::openCommutatorSession(*this, pRootSession);
    ASSERT_TRUE(pCommutator);

    client::MessangerPtr pClient = Helper::getMessanger(pCommutator);
    ASSERT_TRUE(pClient);

    const char*    sServiceName = "SomeBadService";
    const uint32_t nSomeSeq     = 32837;
    Service service = spawnService(pCommutator, sServiceName);

    for (uint32_t nTimeout : {100, 200, 400, 800, 1600, 3200, 4999}) {
        MaybeError error = pClient->sendRequest(
            sServiceName, nSomeSeq, "PING REQ", nTimeout);
        ASSERT_FALSE(error) << "Can't send request: " << *error;

        // Receive request on service side
        client::Messanger::Request request;
        error = service.pSession->waitRequest(request);
        ASSERT_FALSE(error) << "Can't get request: " << *error;

        // Send N responses (only last one is final)
        for (size_t i = 1; i <= 32; ++i) {
            justWait(0.9 * nTimeout);
            const bool isFinal = i == 32;
            const std::string body = String::concat("PING RESP #", i);
            error = service.pSession->sendResponse(request.nSeq, body, isFinal);
            ASSERT_FALSE(error) << "Can't send response #" << i << ": " << *error;

            uint32_t    nResponseSeq;
            std::string nResponseBody;
            error = pClient->waitResponse(nResponseSeq, nResponseBody);
            ASSERT_FALSE(error) << "Can't get response #" << i << ": " << *error;
            ASSERT_EQ(nResponseSeq, nResponseSeq);
            ASSERT_EQ(body, nResponseBody);
        }
    }
}

TEST_F(MessangerTests, SessionTimeout)
{
    Helper::createMessangerModule(*this);

    client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
    ASSERT_TRUE(pRootSession);
    client::ClientCommutatorPtr pCommutator =
        Helper::openCommutatorSession(*this, pRootSession);
    ASSERT_TRUE(pCommutator);

    client::MessangerPtr pClient = Helper::getMessanger(pCommutator);
    ASSERT_TRUE(pClient);

    const char* sServiceName = "SomeBadService";
    const uint32_t nSomeSeq = 4334;
    Service service = spawnService(pCommutator, sServiceName);

    for (uint32_t nTimeout : {100, 200, 400, 800, 1600, 3200, 4999}) {
        MaybeError error = pClient->sendRequest(
            sServiceName, nSomeSeq, "PING REQ", nTimeout);
        ASSERT_FALSE(error) << "Can't send request: " << *error;

        // Receive request on service side
        client::Messanger::Request request;
        error = service.pSession->waitRequest(request);
        ASSERT_FALSE(error) << "Can't get request: " << *error;

        // Wait for timeout
        justWait(1.1 * nTimeout);

        // Try to send response
        error = service.pSession->sendResponse(request.nSeq, "PING RESP");
        ASSERT_FALSE(error) << "Can't send response: " << *error;

        // WRONG_SEQ status should be received
        {
            client::Messanger::SessionStatus status;
            error = service.pSession->waitSessionStatus(status);
            ASSERT_FALSE(error) << "Can't get session status: " << *error;
            ASSERT_EQ(request.nSeq, status.nSeq);
            ASSERT_EQ(spex::IMessanger::WRONG_SEQ, status.eStatus);
        }

        // Client should get "session closed"
        {
            client::Messanger::SessionStatus status;
            error = pClient->waitSessionStatus(status);
            ASSERT_FALSE(error) << "Can't get session status: " << *error;
            ASSERT_EQ(nSomeSeq, status.nSeq);
            ASSERT_EQ(spex::IMessanger::CLOSED, status.eStatus);
        }
    }
}

}  // namespace autotests