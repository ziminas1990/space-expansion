#include <Network/SessionMux.h>
#include <Utils/Clock.h>
#include <Utils/RandomSequence.h>


namespace network {

static uint16_t generateToken(uint16_t old) {
  static utils::RandomSequence tokensStream(time(nullptr));
  uint16_t token = tokensStream.yield16();
  while (token == old || token == 0) {
    // quite unlikely but anyway
    token = tokensStream.yield16();
  }
  return token;
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
  // No need to lock here
  const uint32_t nSessionId    = message.tunnelid();
  const uint32_t nSessionIndex = nSessionId >> 16;

  if (nSessionId && nSessionIndex < m_pOwner->m_sessions.size()) {
    const Session& session = m_pOwner->m_sessions[nSessionIndex];
    if (session.m_nConnectionId == nConnectionId &&
        session.sessionId() == nSessionId) {
      session.m_pHandler->onMessageReceived(nSessionId, message);
    }
  }
}

void SessionMux::Socket::onSessionClosed(uint32_t nConnectionId)
{
  // A client closed the connection
  if (m_pOwner) {
    m_pOwner->closeConnection(nConnectionId);
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
  if (nSessionIdx < m_pOwner->m_sessions.size()) {
    const Session& session = m_pOwner->m_sessions[nSessionIdx];
    assert(session.sessionId() == nSessionId);
    return session.isValid()
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
  m_sessions.reserve(1024);
  // SessionId = 0 should never be used
  m_sessions.push_back(Session());
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
    const uint32_t nSessionId = m_sessions.back().sessionId();
    connection.m_sessions.push_back(nSessionId);
    connection.m_lUp = true;
    return nSessionId;
  }
  assert(!"Invalid connection id");
  return 0;
}

bool SessionMux::closeConnection(uint32_t nConnectionId)
{
  std::lock_guard<std::mutex> guard(m_mutex);
  return closeConnectionLocked(nConnectionId);
}

void SessionMux::onConnectionClosed(uint32_t nConnectionId)
{
  std::lock_guard<std::mutex> guard(m_mutex);
  if (nConnectionId < m_connections.size()) {
    Connection& connection = m_connections[nConnectionId];
    assert(connection.isOpened() && "Has connection been opened?");
    for (uint32_t nSessionId: connection.m_sessions) {
      onSessionClosedLocked(nSessionId);
    }
    connection.closed();
  }
  assert(!"Invalid connection id");
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
  std::lock_guard<std::mutex> guard(m_mutex);
  if (nConnectionId < m_connections.size()) {
    Connection& connection = m_connections[nConnectionId];
    assert(connection.isOpened() && "Has connection been opened?");
    for (uint32_t nSessionId: connection.m_sessions) {
      closeSessionLocked(nSessionId);
    }
    connection.closed();
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

bool SessionMux::isRootSession(const Session& session) const
{
  if (session.m_nConnectionId < m_connections.size()) {
    const Connection& connection = m_connections[session.m_nConnectionId];
    assert(connection.isOpened());
    return connection.isOpened() 
        && connection.getRootSession() == session.sessionId();
  }
  assert(!"Invalid session id");
}

} // network