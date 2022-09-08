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

std::optional<Connection> Helper::openAdditionalSession(
    ModulesTestFixture& env, uint32_t nExistingConnectionId)
{
  std::optional<uint32_t> nSessionId =
      env.m_pPlayer->openAdditionalSession(nExistingConnectionId);
  if (!nSessionId.has_value()) {
    return std::nullopt;
  }

  Connection connection;
  connection.m_nRootSessionId  = *nSessionId;
  connection.m_pCommutatorCtrl = std::make_shared<client::ClientCommutator>(
    env.m_pRouter);
  connection.m_pCommutatorCtrl->attachToChannel(
    env.m_pRouter->openSession(connection.m_nRootSessionId));

  // Connector should also be informed about new connection
  env.m_pConnector->onAdditionalConnection(nExistingConnectionId, *nSessionId);
  return connection;
}

}   // namespace autotests