#include <Autotests/ClientSDK/Router.h>

namespace autotests::client {

Router::SessionPtr Router::openSession(uint32_t nSessionId)
{
  assert(m_sessions.find(nSessionId) == m_sessions.end());
  assert(m_pDownLevel && "Can't create a valid session without downlevel");
  SessionPtr pSession = std::make_shared<Session>(nSessionId);
  pSession->attachToDownlevel(m_pDownLevel);
  pSession->setProceeder(m_fProceeder);
  m_sessions[nSessionId] = pSession;
  return pSession;
}

bool Router::closeSession(uint32_t nSessionId)
{
  auto I = m_sessions.find(nSessionId);
  if (I != m_sessions.end()) {
    m_sessions.erase(I);
    return true;
  }
  return false;
}

void Router::onMessageReceived(spex::Message&& message)
{
  auto I = m_sessions.find(message.tunnelid());
  //std::cout << "Received in " << (message.tunnelid() & 0xFFFF) <<
  //             ":\n" << message.DebugString() << std::endl;
  if (I != m_sessions.end()) {
    SessionPtr pSession = I->second;
    pSession->onMessageReceived(std::move(message));
  } else {
    assert(!"Session not found");
  }
}

}