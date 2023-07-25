#include <Network/SessionMux.h>
#include <Utils/Clock.h>
#include <Utils/RandomSequence.h>

DECLARE_GLOBAL_CONTAINER_CPP(network::SessionMux);

namespace network {

static uint16_t generateToken(uint16_t old) {
  static utils::RandomSequence tokensStream(time(nullptr));
  const uint16_t token = tokensStream.yield16();
  return token > 0 && token != old ? token : generateToken(old);
}

void SessionMux::Session::die()
{
  // 'm_nToken' should NOT be overwritten with 0 (see revive())
  m_nConnectionId    = 0;
  m_pHandler         = nullptr;
  assert(!isValid());
}

void SessionMux::Session::revive(uint32_t           nConnectionId,
                                 IPlayerTerminalPtr pHandler)
{
  assert(!isValid());
  m_nToken        = generateToken(m_nToken);
  m_nConnectionId = nConnectionId;
  m_pHandler      = pHandler;
  assert(isValid());
}

void SessionMux::Socket::closeConnection(uint32_t nConnectionId)
{
  // Server closes the connection
  if (m_pChannel) {
    m_pChannel->closeSession(nConnectionId);
  }
}

void SessionMux::Socket::onMessageReceived(uint32_t nConnectionId,
                                           spex::Message const& message)
{
  // No need to lock anything here
  const uint32_t nSessionId = message.tunnelid();

  //std::cerr << "Received in " << nSessionId << ": " << message.DebugString() << std::endl;

  if (message.choice_case() != spex::Message::kSession) {  // [[likely]]
    const uint32_t nSessionIndex = nSessionId >> 16;
    if (nSessionId && nSessionIndex < m_pOwner->m_sessions.size()) {
      const Session& session = m_pOwner->m_sessions[nSessionIndex];
      if (session.m_nConnectionId == nConnectionId &&
          session.sessionId() == nSessionId) {
        session.m_pHandler->onMessageReceived(nSessionId, message);
      }
    }
  } else {
    // Got an ISessionControl message. It shouldn't be passed to uplevel but should
    // be just handled right here.
    if (message.session().choice_case() == spex::ISessionControl::kClose) {
      closeSession(nSessionId);
    }
  }

  Connection& connection = m_pOwner->m_connections[nConnectionId];
  connection.m_nLastMessageReceivedAt = utils::GlobalClock::running_time();
}

void SessionMux::Socket::onSessionClosed(uint32_t nConnectionId)
{
  // Client closed the connection(!)
  if (m_pOwner) {
    m_pOwner->onConnectionClosed(nConnectionId);
  }
}

void SessionMux::Socket::attachToChannel(IPlayerChannelPtr pChannel)
{
  m_pChannel = pChannel;
}

void SessionMux::Socket::detachFromChannel()
{
  m_pChannel = nullptr;
}

bool SessionMux::Socket::send(uint32_t nSessionId, spex::Message&& message)
{
  const uint16_t nSessionIdx = nSessionId >> 16;
  message.set_tunnelid(nSessionId);
  message.set_timestamp(utils::GlobalClock::now());
  //std::cerr << "Sending in " << nSessionId << ": " << message.DebugString() << std::endl;

  if (nSessionIdx < m_pOwner->m_sessions.size()) {
    const Session& session = m_pOwner->m_sessions[nSessionIdx];
    return session.isValid()
        && session.sessionId() == nSessionId
        && m_pChannel
        && m_pChannel->send(session.m_nConnectionId, std::move(message));
  }
  return false;
}

void SessionMux::Socket::closeSession(uint32_t nSessionId)
{
  // A module closes the session
  m_pOwner->closeSession(nSessionId);
}

bool SessionMux::Socket::isValid() const
{
  return m_pChannel && m_pChannel->isValid();
}

void SessionMux::Socket::attachToTerminal(IPlayerTerminalPtr)
{
  assert(!"Operation makes no sense");
}

void SessionMux::Socket::detachFromTerminal()
{
  assert(!"Operation makes no sense");
}

//==============================================================================
// SessionMux
//==============================================================================

SessionMux::SessionMux(uint8_t nConnectionsLimit)
  : m_connections(nConnectionsLimit), m_pSocket(std::make_shared<Socket>(this))
{
  GlobalObject<SessionMux>::registerSelf(this);
  m_sessions.reserve(1024);
  // SessionId = 0 should never be used
  m_sessions.push_back(Session());
}

SessionMux::~SessionMux()
{
  for (const Session& session: m_sessions) {
    // SessionMux can't be properly destroyed while there is at least one active
    // session. Make sure all sessions are closed before destroying SessionMux.
    assert(!session.isValid());
  }
}

uint32_t SessionMux::addConnection(uint32_t           nConnectionId,
                                   IPlayerTerminalPtr pHandler)
{
  std::lock_guard<std::mutex> guard(m_mutex);
  if (nConnectionId < m_connections.size()) {
    Connection& connection = m_connections[nConnectionId];
    assert(!connection.isOpened() && "Has connection been closed?");
    assert(connection.m_sessions.empty());
    // TODO: try to find a session for reuse, using occpyIndex())?
    m_sessions.emplace_back(Session{
      static_cast<uint16_t>(m_sessions.size()),
      generateToken(0),
      nConnectionId,
      pHandler
    });
    const uint32_t nSessionId   = m_sessions.back().sessionId();
    const uint64_t running_time = utils::GlobalClock::running_time();
    connection.m_sessions.push_back(nSessionId);
    connection.m_lUp = true;
    connection.m_nLastMessageReceivedAt = running_time;
    connection.m_nLastHeartbeatSentAt   = running_time;
    return nSessionId;
  }
  assert(!"Invalid connection id");
  return 0;
}

uint32_t SessionMux::createSession(uint32_t           nParentSessionId,
                                   IPlayerTerminalPtr pHandler)
{
  const uint16_t nParentSessionIndex = nParentSessionId >> 16;

  std::lock_guard<std::mutex> guard(m_mutex);

  if (nParentSessionIndex >= m_sessions.size()) {
    assert(!"Invalid parent session");
    return 0;
  }

  // Get connection id from the specified parent session
  const Session& parentSession = m_sessions[nParentSessionIndex];
  if (!parentSession.isValid() ||
      parentSession.sessionId() != nParentSessionId) {
    // Either parent session is not valid or secret key doesn't match
    assert(!"Invalid parent session");
    return 0;
  }
  const uint32_t nConnectionId = parentSession.m_nConnectionId;
  assert(nConnectionId < m_connections.size());

  // Add a new session
  const uint16_t nSessionIndex = occupyIndex();
  if (nSessionIndex) {
    assert(nSessionIndex < m_sessions.size());

    Session& session = m_sessions[nSessionIndex];
    assert(session.m_nIndex == nSessionIndex);
    session.revive(nConnectionId, pHandler);
    const uint32_t nSessionId = session.sessionId();

    Connection& connection = m_connections[nConnectionId];
    assert(connection.isOpened());
    connection.m_sessions.push_back(nSessionId);

    return nSessionId;
  }
  return 0;
}

bool SessionMux::closeSession(uint32_t nSessionId)
{
  std::lock_guard<std::mutex> guard(m_mutex);
  const uint16_t nSessionIndex = nSessionId >> 16;
  if (nSessionIndex < m_sessions.size()) {
    Session& session = m_sessions[nSessionIndex];
    if (isRootSession(session)) {
      return closeConnectionLocked(session.m_nConnectionId);
    } else {
      return closeSessionLocked(nSessionId);
    }
  } else {
    return false;
  }
}

void SessionMux::checkConnectionsActivity(uint64_t real_now)
{
   // ~1400 ms
  constexpr uint64_t disconnectTimeoutUs = static_cast<uint64_t>(400000 * 3.5);

  const size_t nTotal = m_connections.size();
  for (size_t nConnectionId = 0; nConnectionId < nTotal; ++ nConnectionId) {
    Connection& connection = m_connections[nConnectionId];
    if (!connection.isOpened()) {
      continue;
    }
    if (connection.isTimeToSendHeartbeat(real_now)) {
      const uint64_t nSilencePeriod = real_now - connection.m_nLastMessageReceivedAt;
      if (nSilencePeriod < disconnectTimeoutUs) {  // [[likely]]
        spex::Message heartbeat;
        heartbeat.mutable_session()->set_heartbeat(true);
        m_pSocket->send(connection.getRootSession(), std::move(heartbeat));
        connection.m_nLastHeartbeatSentAt = real_now;
      } else {
        closeConnection(nConnectionId);
        // Note: 'connection' reference is invalidated real_now
      }
    }
  }
}

void SessionMux::onSessionClosed(uint32_t nSessionId)
{
  std::lock_guard<std::mutex> guard(m_mutex);
  onSessionClosedLocked(nSessionId);
}

void SessionMux::onSessionClosedLocked(uint32_t nSessionId)
{
  const uint16_t nSessionIndex = nSessionId >> 16;
  if (!nSessionIndex || nSessionIndex >= m_sessions.size()) {
    assert(!"Invalid session id");
    return;
  }

  Session& session = m_sessions[nSessionIndex];
  if (session.isValid() && session.sessionId() == nSessionId) {
    if (session.m_pHandler) {
      session.m_pHandler->onSessionClosed(session.sessionId());
    }
    session.die();
    m_indexesToReuse.push_back(session.m_nIndex);
  }
}

bool SessionMux::closeConnection(uint32_t nConnectionId)
{
  std::lock_guard<std::mutex> guard(m_mutex);
  return closeConnectionLocked(nConnectionId);
}

void SessionMux::markAllConnectionsAsClosed()
{
  std::lock_guard<std::mutex> guard(m_mutex);
  for (size_t nConnectionId = 0; nConnectionId < m_connections.size(); ++nConnectionId) {
    Connection& connection = m_connections[nConnectionId];
    if (connection.isOpened()) {
      onConnectionClosedLocked(nConnectionId);
    }
  }
}

void SessionMux::onConnectionClosed(uint32_t nConnectionId)
{
  std::lock_guard<std::mutex> guard(m_mutex);
  onConnectionClosedLocked(nConnectionId);
}

void SessionMux::onConnectionClosedLocked(uint32_t nConnectionId)
{
  if (nConnectionId < m_connections.size()) {
    Connection& connection = m_connections[nConnectionId];
    assert(connection.isOpened() && "Has connection been opened?");
    for (uint32_t nSessionId : connection.m_sessions) {
      onSessionClosedLocked(nSessionId);
    }
    connection.closed();
  } else {
    assert(!"Invalid connection id");
  }
}

uint16_t SessionMux::occupyIndex()
{
  if (!m_indexesToReuse.empty()) {
    const uint16_t nIndex = m_indexesToReuse.back();
    m_indexesToReuse.pop_back();
    return nIndex;
  } else if (m_sessions.size() < 0xFFFF) {
    const uint16_t index = static_cast<uint16_t>(m_sessions.size());
    m_sessions.push_back(Session{index, 0, 0, nullptr});
    return m_sessions.size() - 1;
  }
  return 0;
}

bool SessionMux::closeConnectionLocked(uint32_t nConnectionId)
{
  if (nConnectionId < m_connections.size()) {
    Connection& connection = m_connections[nConnectionId];
    assert(connection.isOpened() && "Has connection been opened?");
    for (uint32_t nSessionId: connection.m_sessions) {
      closeSessionLocked(nSessionId);
    }
    connection.closed();
    m_pSocket->closeConnection(nConnectionId);
    return true;
  }
  assert(!"Invalid connection id");
  return false;
}

bool SessionMux::closeSessionLocked(uint32_t nSessionId)
{
  const uint16_t nSessionIndex = nSessionId >> 16;

  if (!nSessionIndex || nSessionIndex >= m_sessions.size()) {
    assert(!"Invalid session id");
    return false;
  }

  Session& session = m_sessions[nSessionIndex];
  if (session.isValid() && session.sessionId() == nSessionId) {
    // Notify the client and close the session
    spex::Message message;
    message.mutable_session()->set_closed_ind(true);
    m_pSocket->send(nSessionId, std::move(message));

    if (session.m_pHandler) {
      session.m_pHandler->onSessionClosed(session.sessionId());
    }

    session.die();
    m_indexesToReuse.push_back(session.m_nIndex);
    return true;
  }

  return false;
}

bool SessionMux::isRootSession(const Session& session) const
{
  if (session.m_nConnectionId < m_connections.size()) {
    const Connection& connection = m_connections[session.m_nConnectionId];
    assert(connection.isOpened());
    return connection.isOpened()
        && connection.getRootSession() == session.sessionId();
  }
  assert(!"Invalid session id");
  return false;
}

bool SessionMuxManager::prephare(uint16_t, uint32_t, uint64_t)
{
  using SessionMuxContainer = utils::GlobalContainer<SessionMux>;

  // NOTE: can't use ingame time for connectivity check, because it may cause problems
  // in case GlobalClock is in Debug mode and accelerated hundreds of times. In this case
  // network latency may break connectivity check
  const uint64_t real_now = utils::GlobalClock::running_time();

  for (SessionMux* pSessionMux : SessionMuxContainer::AllInstancies()) {
    if (pSessionMux) {
      pSessionMux->checkConnectionsActivity(real_now);
    }
  }
  return false;  // No need to proceed stage, everything is already done
}

void SessionMuxManager::proceed(uint16_t, uint32_t, uint64_t)
{
  assert("!Should never be called");
}

} // network
