#include "Autotests/ClientSDK/Modules/ClientCommutator.h"
#include <Autotests/Modules/Helper.h>

#include <Modules/Game/Game.h>
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

void Helper::createMessangerModule(
    ModulesTestFixture& env,
    world::PlayerPtr    pOwner)
{
  pOwner = pOwner ? pOwner : env.m_pPlayer;

  // Messanger should be created on "server" side and attached to player's
  // commutator
  modules::MessangerPtr pMessanger = std::make_shared<modules::Messanger>(
    "Messanger", pOwner
  );

  env.m_pPlayer->testAccess().setMessanger(pMessanger);
  // Now messanger can be reached using client commutator
  // (use Helper::getMessanger() call)
}

client::MessangerPtr Helper::getMessanger(client::ClientCommutatorPtr pCommutator)
{
  client::Router::SessionPtr pSession = Helper::openSession(pCommutator, "Messanger");

  if (pSession) {
    client::MessangerPtr pMessanger = std::make_shared<client::Messanger>();
    pMessanger->attachToChannel(pSession);
    return pMessanger;
  }

  return client::MessangerPtr();
}

void Helper::createGameModule(
    ModulesTestFixture& env,
    world::PlayerPtr    pOwner)
{
  pOwner = pOwner ? pOwner : env.m_pPlayer;

  modules::GamePtr pGame = std::make_shared<modules::Game>("Game", pOwner);
  env.m_pPlayer->testAccess().setGame(pGame);
}

client::GamePtr Helper::getGame(client::ClientCommutatorPtr pCommutator)
{
  client::Router::SessionPtr pSession = Helper::openSession(pCommutator, "Game");

  if (pSession) {
    client::GamePtr pGame = std::make_shared<client::Game>();
    pGame->attachToChannel(pSession);
    return pGame;
  }

  return client::GamePtr();
}

}   // namespace autotests