#include <Network/SessionMux.h>
#include <Utils/Clock.h>

namespace network {

void SessionMux::Session::removeChild(uint32_t nChildSessionId)
{
  // This requires a lnear search, but it shouldn't be critical
  for (size_t i = 0; i < m_children.size(); ++i) {
    if (m_children[i] == nChildSessionId) {
      m_children[i] = m_children.back();
      m_children.pop_back();
    }
  }
}

void SessionMux::Session::die()
{
  // 'm_nSecretKey' should NOT be overwritten with 0 (see revive())
  m_nConnectionId    = 0;
  m_pHandler         = nullptr;
  m_nParentSessionId = 0;
  m_children.clear();
  assert(!isValid());
}

void SessionMux::Session::revive(uint32_t           nConnectionId,
                                 uint32_t           nParentSessionId,
                                 IPlayerTerminalPtr pHandler)
{
  assert(!isValid());
  ++m_nSecretKey;
  m_nConnectionId    = nConnectionId;
  m_nParentSessionId = nParentSessionId;
  m_pHandler         = pHandler;
  m_children.clear();
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
  const uint32_t nSessionIndex = nSessionId & 0xFFFF;

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
  m_pOwner->closeConnection(nConnectionId);
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
  const uint16_t nSessionIdx = nSessionId & 0xFFFF;
  message.set_tunnelid(nSessionId);
  message.set_timestamp(utils::GlobalClock::now());
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
  assert(!"Operation makes no sence");
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
    assert(!connection.m_nRootSessionId
           && "Connection has not been closed?");
    // TODO: try to find a session for reuse, using occpyIndex())?
    m_sessions.emplace_back(Session{
      static_cast<uint16_t>(m_sessions.size()),
      1,  // A secret key (starts from 1)
      nConnectionId,
      0,  // parentSessionId, should be 0 here
      pHandler,
      {}
    });
    connection.m_nRootSessionId = m_sessions.back().sessionId();
    return connection.m_nRootSessionId;
  }
  assert(!"Invalid connection id");
  return 0;
}

bool SessionMux::closeConnection(uint32_t nConnectionId)
{
  std::lock_guard<std::mutex> guard(m_mutex);
  if (nConnectionId < m_connections.size()) {
    Connection& connection = m_connections[nConnectionId];
    assert(connection.isValid() && "Connection has not been opened?");
    onSessionClosed(connection.m_nRootSessionId);
    connection.m_nRootSessionId = 0;
    return true;
  }
  assert(!"Invalid connection id");
  return false;
}

uint32_t SessionMux::createSession(uint32_t           nParentSessionId,
                                   IPlayerTerminalPtr pHandler)
{
  const uint16_t nParentSessionIndex = nParentSessionId & 0xFFFF;

  std::lock_guard<std::mutex> guard(m_mutex);

  if (nParentSessionIndex >= m_sessions.size()) {
    assert(!"Invalid parent session");
    return 0;
  }

  // Get connection id from the specified parent session
  Session& parentSession = m_sessions[nParentSessionIndex];
  if (!parentSession.isValid() ||
      parentSession.sessionId() != nParentSessionId) {
    // Either parent session is not valid or secret key doesn't match
    assert(!"Invalid parent session");
    return 0;
  }
  const uint32_t nConnectionId = parentSession.m_nConnectionId;

  // Add a new session
  const uint16_t nSessionIndex = occupyIndex();
  if (nSessionIndex) {
    if (nSessionIndex < m_sessions.size()) {
      // Reusing previously closed session
      Session& session = m_sessions[nSessionIndex];
      assert(session.m_nIndex == nSessionIndex);
      session.revive(nConnectionId, nParentSessionId, pHandler);
    } else {
      m_sessions.push_back(Session{
        nSessionIndex, 1, nConnectionId, nParentSessionId, pHandler, {}
      });
    }
    return m_sessions.back().sessionId();
  }
  return 0;
}

bool SessionMux::closeSession(uint32_t nSessionId)
{
  std::lock_guard<std::mutex> guard(m_mutex);
  return onSessionClosed(nSessionId);
}

void SessionMux::attach(IPlayerChannelPtr pChannel)
{
  m_pSocket->attachToChannel(pChannel);
}

void SessionMux::detach()
{
  m_pSocket->detachFromChannel();
}

uint16_t SessionMux::occupyIndex()
{
  if (!m_indexesToReuse.empty()) {
    const uint16_t nIndex = m_indexesToReuse.back();
    m_indexesToReuse.pop_back();
    return nIndex;
  }
  const size_t candidate = m_sessions.size();
  return candidate <= 0xFFFF ? static_cast<uint16_t>(candidate) : 0;
}

bool SessionMux::onSessionClosed(uint32_t nSessionId, bool lIsRecursive)
{
  const uint16_t nSessionIndex = nSessionId & 0xFFFF;

  if (!nSessionIndex || nSessionIndex >= m_sessions.size()) {
    assert(!"Invalid session id");
    return false;
  }

  Session& session = m_sessions[nSessionIndex];
  if (session.isValid() && session.sessionId() == nSessionId) {
    if (!lIsRecursive) {
      // Notify parent session
      const uint16_t nParentSessionIndex = session.m_nParentSessionId & 0xFFFF;
      if (nParentSessionIndex < m_sessions.size()) {
        Session& parentSession = m_sessions[nParentSessionIndex];
        if (parentSession.isValid() &&
            parentSession.sessionId() == session.m_nParentSessionId) {
          parentSession.removeChild(nSessionId);
        }
      }
    }

    // First close child sessions then close this session
    for (uint32_t nChildSessionId : session.m_children) {
      onSessionClosed(nChildSessionId, true);
    }

    // Notify the client and close the session
    spex::Message message;
    message.mutable_commutator()->set_close_tunnel_ind(true);
    m_pSocket->send(nSessionId, std::move(message));

    session.m_pHandler->onSessionClosed(session.sessionId());

    // If a root session is closed, connection should be closed as well
    if (session.m_nParentSessionId == 0) {
      m_pSocket->closeConnection(session.m_nConnectionId);
    }

    session.die();
    m_indexesToReuse.push_back(session.m_nIndex);
    return true;
  }

  return false;
}

} // network