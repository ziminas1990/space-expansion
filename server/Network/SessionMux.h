#pragma once

#include <mutex>

#include <Network/Interfaces.h>

namespace network {

class SessionMux {
private:

  struct Connection {
    uint32_t m_nDefaultSessionId = 0;

    bool isValid() const { return m_nDefaultSessionId != 0; }
    bool closed() { m_nDefaultSessionId = 0; }
  };

  struct Session {
    uint16_t              m_nIndex           = 0;
    uint16_t              m_nSecretKey       = 0;
    uint32_t              m_nConnectionId    = 0;
    uint32_t              m_nParentSessionId = 0;
    IPlayerTerminalPtr    m_pHandler         = nullptr;
    std::vector<uint32_t> m_children;

    bool     isValid()   const { return m_pHandler != nullptr; }
    uint32_t sessionId() const { return m_nSecretKey << 16 + m_nIndex; }
    
    void removeChild(uint32_t nChildSessionId);
    void die();
    void revive(uint32_t           nRootSessionId,
                uint32_t           nParentSession,
                IPlayerTerminalPtr pHandler);

  };


  class Socket : public IPlayerChannel, public IPlayerTerminal
  {
  private:
    SessionMux*       m_pOwner;
    IPlayerChannelPtr m_pChannel;

  public:
    Socket(SessionMux* pOwner) : m_pOwner(pOwner) {}

    // Overrides from IPlayerTerminal
    bool openSession(uint32_t nClnRootSessionIdientId) override { return true; }
    void onMessageReceived(uint32_t nRootSessionId, spex::Message const& message) override;
    void onSessionClosed(uint32_t nRootSessionId) override;
    void attachToChannel(IPlayerChannelPtr pChannel) override;
    void detachFromChannel() override;

    // Overrides from IPlayerChannel
    bool send(uint32_t nSessionId, spex::Message const& message) const override;
    void closeSession(uint32_t nSessionId) override;
    bool isValid() const override;
    void attachToTerminal(IPlayerTerminalPtr pTerminal) override;
    void detachFromTerminal() override;

  };

private:
  std::mutex               m_mutex;
  std::vector<Connection>  m_connections;
  std::vector<Session>     m_sessions;
  std::vector<uint16_t>    m_indexesToReuse;
  std::shared_ptr<Socket>  m_pSocket;

public:
  SessionMux(uint8_t nConnectionsLimit);

  bool addConnection(uint32_t nConnectionId, IPlayerTerminalPtr pHandler);
  bool closeConnection(uint32_t nConnectionId);

  uint32_t createSession(uint32_t nParentSessionId, 
                         IPlayerTerminalPtr pHandler);
  void closeSession(uint32_t nSessionId);

  IPlayerChannelPtr getChannel()    const { return m_pSocket; }
  IPlayerChannelPtr getEntryPoint() const { return m_pSocket; }

private:

  uint16_t occupyIndex();

  void onConnectionClosed(uint32_t nConnectionId);

  void onSessionClosed(uint32_t nSessionId);

  void closeSessionLocked(uint32_t nSessionId);
};

}  // namespace network