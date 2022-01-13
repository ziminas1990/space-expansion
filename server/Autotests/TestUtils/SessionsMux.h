#pragma once

#include <Network/Interfaces.h>
#include <Autotests/ClientSDK/Interfaces.h>

namespace autotests {

class Session : public client::IPlayerChannel
{
private:
  uint32_t                   m_nSessionId;
  network::IPlayerChannelPtr m_pChannel;
  client::IPlayerTerminalPtr m_pTerminal;

public:
  Session(uint32_t nSessionId,
          network::IPlayerChannelPtr pChannel,
          client::IPlayerTerminalPtr pTerminal)
    : m_nSessionId(nSessionId)
    , m_pChannel(pChannel)
    , m_pTerminal(pTerminal)
  {}

  bool send(spex::Message const& message) override {
    return m_pChannel->send(m_nSessionId, message);
  }

  void close() {
    m_pChannel = nullptr;
    m_pTerminal->detachDownlevel();
    m_pTerminal = nullptr;
  }

  void onMessageReceived(spex::Message&& message) {
    m_pTerminal->onMessageReceived(std::move(message));
  }
};

using SessionPtr = std::shared_ptr<Session>;

class SessionMux : public network::IPlayerTerminal
{
  // Implements autotests::client::IPlayerTerminal interface using
  // network::IPlayerChannel as downlevel.
  // The main difference between 'autotests::client' and 'network' interfaces
  // is that first one doesn't operate with 'sessionId', but secund one does.

private:
  std::map<uint32_t, SessionPtr> m_sessions;
  network::IPlayerChannelPtr     m_pChannel;

public:

  SessionPtr openSession(uint32_t nSessionId,
                         client::IPlayerTerminalPtr pClient)
  {
    assert(m_pChannel);
    assert(m_sessions.find(nSessionId) == m_sessions.end());
    SessionPtr pSession = std::make_shared<Session>(
          nSessionId, m_pChannel, pClient);
    pClient->attachToDownlevel(pSession);
    m_sessions.emplace(nSessionId, pSession);
    return pSession;
  }

  void closeSession(uint32_t nSessionId) {
    auto itSession = m_sessions.find(nSessionId);
    if (itSession != m_sessions.end()) {
      itSession->second->close();
      m_sessions.erase(itSession);
    }
  }

  void closeAllSession() {
    for (auto& [nSessionId, pSession]: m_sessions) {
      pSession->close();
    }
    m_sessions.clear();
  }

  bool openSession(uint32_t) override {
    assert(nullptr == "Server will never request to open new session");
    return false;
  }

  void onMessageReceived(
      uint32_t nSessionId, spex::Message const& message) override
  {
    auto itSession = m_sessions.find(nSessionId);
    if (itSession != m_sessions.end()) {
      SessionPtr pSession = itSession->second;
      pSession->onMessageReceived(spex::Message(message));
    }
  }

  void onSessionClosed(uint32_t) override {
    assert(nullptr == "Use 'closeSession()' instead!");
  }

  void attachToChannel(network::IPlayerChannelPtr pChannel) override {
    assert(m_sessions.empty());
    m_pChannel = pChannel;
  }

  void detachFromChannel() override {
    assert(m_sessions.empty());
    m_pChannel = nullptr;
  }

};

using SessionMuxPtr = std::shared_ptr<SessionMux>;

} // namespace autotests
