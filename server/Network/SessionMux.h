#pragma once

#include <mutex>

#include <Network/Interfaces.h>

namespace network {

class SessionMux
{
private:

  struct Connection {
    Connection() {
      m_sessions.reserve(1024);
    }

    bool                  m_lUp = false;
    std::vector<uint32_t> m_sessions;

    bool isOpened() const { return m_lUp; }
    void closed() {
      m_lUp = false;
      m_sessions.clear();
    }
  };

  struct Session {
    uint16_t              m_nIndex           = 0;
    uint16_t              m_nToken           = 0;
    uint32_t              m_nConnectionId    = 0;
    IPlayerTerminalPtr    m_pHandler         = nullptr;

    bool     isValid()   const { return m_pHandler != nullptr; }
    uint32_t sessionId() const { return (m_nIndex << 16) + m_nToken; }
    
    void die();
    void revive(uint32_t           nRootSessionId,
                IPlayerTerminalPtr pHandler);

  };


  class Socket : public IPlayerChannel, public IPlayerTerminal
  {
  private:
    SessionMux*       m_pOwner;
    IPlayerChannelPtr m_pChannel;

  public:
    Socket(SessionMux* pOwner) : m_pOwner(pOwner) {}

    void closeConnection(uint32_t nConnectionId);

    // Overrides from IPlayerTerminal
    bool openSession(uint32_t) override { return true; }
    void onMessageReceived(uint32_t nConnectionId,
                           spex::Message const& message) override;
    void onSessionClosed(uint32_t nConnectionId) override;
    void attachToChannel(IPlayerChannelPtr pChannel) override;
    void detachFromChannel() override;

    // Overrides from IPlayerChannel
    bool send(uint32_t nSessionId, spex::Message&& message) override;
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
  SessionMux(uint8_t nConnectionsLimit = 8);

  // Create a new connection and return it's root session id
  uint32_t addConnection(uint32_t nConnectionId, IPlayerTerminalPtr pHandler);
  bool closeConnection(uint32_t nConnectionId);

  uint32_t createSession(uint32_t nParentSessionId, 
                         IPlayerTerminalPtr pHandler);
  bool closeSession(uint32_t nSessionId);

  IPlayerChannelPtr  asChannel()  const { return m_pSocket; }
  IPlayerTerminalPtr asTerminal() const { return m_pSocket; }

private:
  uint16_t occupyIndex();

  bool closeSessionLocked(uint32_t nSessionId);
};

using SessionMuxPtr = std::shared_ptr<SessionMux>;

}  // namespace network