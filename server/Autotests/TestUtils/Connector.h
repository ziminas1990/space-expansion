#pragma once

#include <type_traits>

#include <Network/Interfaces.h>
#include <Autotests/ClientSDK/Interfaces.h>

namespace autotests {

// This class provides a way to connect client::Router and network::SessionMux
// with each other as if they were communicating through the real network.
//
// Connector must be aware about all connections and sessions, opened
// currently, and, in additional, how sessions map to connections. On the other
// hand, it should be transparent to client code. Once client opens a new
// session, it shouldn't be abliged to register it in connector. That is why
// Connector hooks some ICommutator and ISessionControl messages
template<typename FrameType>
class Connector:
    public network::IChannel<FrameType>,
    public client::IChannel<FrameType>
{
public:
  using IServerTerminalPtr = network::ITerminalPtr<FrameType>;
  using IClientTerminalPtr = client::ITerminalPtr<FrameType>;

private:
  IServerTerminalPtr m_pServiceSide;
  IClientTerminalPtr m_pClientSide;

  // Key - sessionId, value - connectionId
  std::map<uint32_t, uint32_t> m_session2conection;
  // Key - connectionId, value - sessions
  std::map<uint32_t, std::set<uint32_t>> m_connection2sessions;

public:

  void onNewConnection(uint32_t nConnectionId, uint32_t nRootSessionId) {
    assert(m_connection2sessions.find(nConnectionId) == m_connection2sessions.end());
    assert(m_session2conection.find(nRootSessionId) == m_session2conection.end());
    m_connection2sessions[nConnectionId] = {nRootSessionId};
    m_session2conection[nRootSessionId] = nConnectionId;
  }

  void onAdditionalConnection(uint32_t nExistingConnectionId, uint32_t nSessionId) {
    assert(m_connection2sessions.find(nExistingConnectionId) != m_connection2sessions.end());
    assert(m_session2conection.find(nSessionId) == m_session2conection.end());
    m_connection2sessions[nExistingConnectionId].insert(nSessionId);
    m_session2conection[nSessionId] = nExistingConnectionId;
  }

  // Overrides network::IChannel<FrameType>
  bool send(uint32_t nConnectionId, FrameType&& frame) override
  {
    checkPair(nConnectionId, frame.tunnelid());
    if (m_pClientSide) {
      // Handle 'open_tunnel_report' or 'close_ind' messages:
      if constexpr (std::is_same_v<FrameType, spex::Message>) {
        switch (frame.choice_case()) {
          case spex::Message::kCommutator:
            onCommutatorMessage(nConnectionId, frame.commutator());
            break;
          case spex::Message::kSession:
            onSessionMessage(nConnectionId,
                             frame.tunnelid(),
                             frame.session());
            break;
          default:
            break;
        }
      }

      m_pClientSide->onMessageReceived(FrameType(frame));
      return true;
    }
    return false;
  }

  // Should only be called by server side (SessionMux). From server's point of
  // view, each session is a connection.
  void closeSession(uint32_t nConnectionId) override
  {
    auto it = m_connection2sessions.find(nConnectionId);
    assert(it != m_connection2sessions.end());
    for (uint32_t nSessionId: it->second) {
      checkPair(nConnectionId, nSessionId);
      m_session2conection.erase(nSessionId);
    }
    m_connection2sessions.erase(it);
  }

  bool isValid() const override {
    return m_pClientSide != IClientTerminalPtr();
  }

  void attachToTerminal(IServerTerminalPtr pTerminal) override {
    m_pServiceSide = pTerminal;
  }

  // Overrides client::IChannel<FrameType>
  bool send(FrameType&& message) override {
    auto it = m_session2conection.find(message.tunnelid());
    assert(it != m_session2conection.end());
    if (m_pServiceSide) {
      m_pServiceSide->onMessageReceived(it->second, message);
      return true;
    }
    return false;
  }

  void attachToTerminal(IClientTerminalPtr pTerminal) override {
    m_pClientSide = pTerminal;
  }

  // Overrides both network::IChannel<FrameType> & client::IChannel<FrameType>
  void detachFromTerminal() override {
    if (m_pServiceSide) {
      m_pServiceSide->detachFromChannel();
      m_pServiceSide.reset();
    }
    if (m_pClientSide) {
      m_pClientSide->detachDownlevel();
      m_pClientSide.reset();
    }
  }

private:
  void checkPair(uint32_t nConnectionId, uint32_t nSessionId)
  {
    {
      auto it = m_session2conection.find(nSessionId);
      assert(it != m_session2conection.end());
      assert(it->second == nConnectionId);
    }
    {
      auto it = m_connection2sessions.find(nConnectionId);
      assert(it != m_connection2sessions.end());
      assert(it->second.find(nSessionId) != it->second.end());
    }
  }

  void onNewSession(uint32_t nSessionId, uint32_t nConnectionId) {
    assert(m_session2conection.find(nSessionId) == m_session2conection.end());
    m_session2conection[nSessionId] = nConnectionId;

    auto it = m_connection2sessions.find(nConnectionId);
    assert(it != m_connection2sessions.end());
    assert(it->second.find(nSessionId) == it->second.end());
    it->second.insert(nSessionId);
  }

  void onCommutatorMessage(uint32_t nConnectionId,
                           const spex::ICommutator& message)
  {
    // If client openes a new session, we should associate it's id
    // with a corresponding connection
    if (message.choice_case() == spex::ICommutator::kOpenTunnelReport) {
      const uint32_t nSessionId = message.open_tunnel_report();
      onNewSession(nSessionId, nConnectionId);
    }
  }

  void onSessionMessage(uint32_t nConnectionId,
                        uint32_t nSessionId,
                        const spex::ISessionControl& message)
  {
    // If an existing session is closed, we should remove it from internal
    // session <-> connection mappings
    if (message.choice_case() == spex::ISessionControl::kClosedInd) {
      checkPair(nConnectionId, nSessionId);
      m_session2conection.erase(nSessionId);
      m_connection2sessions[nConnectionId].erase(nSessionId);
    }
  }
};


template<typename FrameType>
using ConnectorPtr = std::shared_ptr<Connector<FrameType>>;

template<typename FrameType>
using ConnectorWeakPtr = std::weak_ptr<Connector<FrameType>>;

using PlayerConnector        = Connector<spex::Message>;
using PlayerConnectorPtr     = std::shared_ptr<PlayerConnector>;
using PlayerConnectorWeakPtr = std::weak_ptr<PlayerConnector>;

using AdminConnector        = Connector<admin::Message>;
using AdminConnectorPtr     = std::shared_ptr<AdminConnector>;
using AdminConnectorWeakPtr = std::weak_ptr<AdminConnector>;

} // namespace autotests
