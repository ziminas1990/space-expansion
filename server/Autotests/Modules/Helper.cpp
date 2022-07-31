#include <Autotests/Modules/Helper.h>

#include <Network/SessionMux.h>

namespace autotests {

Connection Helper::connect(ModulesTestFixture& env,
                           uint32_t            nConnectionId)
{
  Connection connection;
  connection.m_nRootSessionId  = env.m_pPlayer->onNewConnection(nConnectionId);
  connection.m_pCommutatorCtrl = std::make_shared<client::ClientCommutator>(
    env.m_pRouter);
  connection.m_pCommutatorCtrl->attachToChannel(
    env.m_pRouter->openSession(connection.m_nRootSessionId));

  // Connector should also be informed about new connection
  env.m_pConnector->onNewConnection(nConnectionId, connection.m_nRootSessionId);
  return connection;
}

}   // namespace autotests