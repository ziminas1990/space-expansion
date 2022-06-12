#pragma once

#include <Network/Interfaces.h>
#include <Autotests/ClientSDK/Interfaces.h>

namespace autotests {

template<typename FrameType>
class Connector:
    public network::IChannel<FrameType>,
    public client::IChannel<FrameType>
{
public:
  using IServerTerminalPtr = network::ITerminalPtr<FrameType>;
  using IClientTerminalPtr = client::ITerminalPtr<FrameType>;

private:
  uint32_t           m_nConnectionId;
  IServerTerminalPtr m_pServiceSide;
  IClientTerminalPtr m_pClientSide;

public:

  Connector(uint32_t nConnectionId) : m_nConnectionId(nConnectionId) {}

  // Overrides network::IChannel<FrameType>
  bool send(uint32_t nSessionId, FrameType&& frame) override
  {
    assert(nSessionId == m_nConnectionId);
    if (m_pClientSide) {
      m_pClientSide->onMessageReceived(FrameType(frame));
      return true;
    }
    return false;
  }

  void closeSession(uint32_t nSessionId) override
  {
    assert(nSessionId == m_nConnectionId);
  }

  bool isValid() const override {
    return m_pClientSide != IClientTerminalPtr();
  }

  void attachToTerminal(IServerTerminalPtr pTerminal) override {
    m_pServiceSide = pTerminal;
  }

  // Overrides client::IChannel<FrameType>
  bool send(FrameType&& message) override {
    if (m_pServiceSide) {
      m_pServiceSide->onMessageReceived(m_nConnectionId, message);
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
};


template<typename FrameType>
using ConnectorPtr = std::shared_ptr<Connector<FrameType>>;

template<typename FrameType>
class ConnectorGuard {
private:
  ConnectorPtr<FrameType>          m_pConnector;
  network::ITerminalPtr<FrameType> m_pServerSide;
  client::ITerminalPtr<FrameType>  m_pClientSide;

public:
  ConnectorGuard() = default;
  ConnectorGuard(const ConnectorGuard<FrameType>& other) = delete;
  ConnectorGuard(ConnectorGuard<FrameType>&& other) = default;
  ~ConnectorGuard() {
    if (m_pConnector) {
      m_pConnector->detachFromTerminal();
    }
  }

  void link(ConnectorPtr<FrameType> pConnector,
            network::ITerminalPtr<FrameType> pServerSide,
            client::ITerminalPtr<FrameType> pClientSide)
  {
    m_pConnector  = pConnector;
    m_pServerSide = pServerSide;
    m_pClientSide = pClientSide;

    m_pConnector->attachToTerminal(m_pServerSide);
    m_pConnector->attachToTerminal(m_pClientSide);
    m_pServerSide->attachToChannel(m_pConnector);
    m_pClientSide->attachToDownlevel(m_pConnector);
  }

};

using PlayerConnector    = Connector<spex::Message>;
using PlayerConnectorPtr = std::shared_ptr<PlayerConnector>;

using AdminConnector    = Connector<admin::Message>;
using AdminConnectorPtr = std::shared_ptr<AdminConnector>;

using PlayerConnectorGuard = ConnectorGuard<spex::Message>;
using AdminConnectorGuard  = ConnectorGuard<admin::Message>;

} // namespace autotests
