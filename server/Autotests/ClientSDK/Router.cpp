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

bool Router::hasSession(uint32_t nSessionId) const
{
  return m_sessions.find(nSessionId) != m_sessions.end();
}

void Router::onMessageReceived(spex::Message&& message)
{
  auto I = m_sessions.find(message.tunnelid());
  // std::cout << "Received in " << (message.tunnelid() >> 16) <<
  //             ":\n" << message.DebugString() << std::endl;
  if (I != m_sessions.end()) {
    SessionPtr pSession = I->second;
    if (message.choice_case() == spex::Message::kSession) {
      if (message.session().choice_case() == spex::ISessionControl::kClosedInd) {
        m_sessions.erase(I);
      }
    }
    pSession->onMessageReceived(std::move(message));
  } else {
    assert(!"Session not found");
  }
}

}