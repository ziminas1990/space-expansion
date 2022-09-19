#include "Autotests/ClientSDK/Modules/ClientCommutator.h"
#include <Autotests/Modules/Helper.h>

#include <Network/SessionMux.h>

namespace autotests {

client::RootSessionPtr Helper::connect(ModulesTestFixture& env,
                                       uint32_t            nConnectionId)
{
  const uint32_t nRootSessionId = env.m_pPlayer->onNewConnection(nConnectionId);
  client::RootSessionPtr pSession = std::make_shared<client::RootSession>();

  pSession->attachToChannel(env.m_pRouter->openSession(nRootSessionId));

  // Connector should also be informed about new connection
  env.m_pConnector->onNewConnection(nConnectionId, nRootSessionId);
  return pSession;
}

client::ClientCommutatorPtr
Helper::openCommutatorSession(ModulesTestFixture&    env,
                              client::RootSessionPtr pRootSession)
{
  uint32_t nSessionId = 0;
  if (!pRootSession->openCommutatorSession(nSessionId)) {
    return nullptr;
  }

  client::ClientCommutatorPtr pCommutator =
      std::make_shared<client::ClientCommutator>(env.m_pRouter);
  pCommutator->attachToChannel(env.m_pRouter->openSession(nSessionId));
  return pCommutator;
}

}   // namespace autotests