#pragma once

#include <mutex>

#include <Network/Interfaces.h>
#include <Conveyor/IAbstractLogic.h>
#include <Utils/GlobalContainer.h>

namespace network {


// SessionMux provides a way to forward incoming messages to appropriate
// handler. Each player has it's own instance of SessionMux. It is shared
// by all UDP player's UDP connections.
// Each UDP connection is represented by SessionMux.Connection instance.
// Each connection may have a number of sessions. First session, created
// when connection is opened, called "root session". If root session is closed,
// all other sessions will be closed as well and connection will be closed.
class SessionMux : public utils::GlobalObject<SessionMux>
{
private:

  struct Connection {
    Connection() {
      m_sessions.reserve(1024);
    }

    bool                  m_lUp = false;
    std::vector<uint32_t> m_sessions;
    // When was the last valid message received from client?
    uint64_t m_nLastMessageReceivedAt = 0;
    // WHen was the last heartbeat sent?
    uint64_t m_nLastHeartbeatSentAt   = 0;

    bool isOpened() const { return m_lUp; }

    uint32_t getRootSession() const {
      assert(isOpened() && !m_sessions.empty());
      return m_sessions.front();
    }

    void closed() {
      m_lUp = false;
      m_sessions.clear();
      m_nLastMessageReceivedAt = 0;
      m_nLastHeartbeatSentAt = 0;
    }

    bool isTimeToSendHeartbeat(uint64_t now) const {
      constexpr uint64_t heartbeatTimeoutUs  = 400000; // 400ms
      return heartbeatTimeoutUs <= (now - m_nLastMessageReceivedAt)
          && heartbeatTimeoutUs <= (now - m_nLastHeartbeatSentAt);
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
    bool sendHeartbeat(uint32_t nConnectionId);

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
  SessionMux(uint8_t nConnectionsLimit = 16);

  // Create a new connection and return it's root session id
  uint32_t addConnection(uint32_t nConnectionId, IPlayerTerminalPtr pHandler);
  bool closeConnection(uint32_t nConnectionId);
  void onConnectionClosed(uint32_t nConnectionId);

  uint32_t createSession(uint32_t nParentSessionId,
                         IPlayerTerminalPtr pHandler);
  // Close session, specified by 'nSessionId'. If session is a root session
  // of some connection, close a connection and all sessions, related with it.
  bool closeSession(uint32_t nSessionId);

  IPlayerChannelPtr  asChannel()  const { return m_pSocket; }
  IPlayerTerminalPtr asTerminal() const { return m_pSocket; }

  // Check if there is any connection, that had no activity (incomnig messages)
  // for some period of time. For such connection send a 'heartbeat' message.
  // If inactivity exceeds 2 seconds, close the connection.
  void checkConnectionsActivity(uint64_t now);

  virtual size_t getCooldownTimeUs() const { return 100 * 1000; }

private:
  uint16_t occupyIndex();

  bool closeConnectionLocked(uint32_t nConnectionId);

  bool closeSessionLocked(uint32_t nSessionId);
  void onSessionClosedLocked(uint32_t nSessionId);

  bool isRootSession(const Session& session) const;
};

class SessionMuxManager : public conveyor::IAbstractLogic
{
public:
  uint16_t getStagesCount() override { return 1; }
  bool prephare(uint16_t nStageId, uint32_t nIntervalUs, uint64_t now) override;
  void proceed(uint16_t nStageId, uint32_t nIntervalUs, uint64_t now) override;
};

}  // namespace network