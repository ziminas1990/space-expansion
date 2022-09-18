#include "Player.h"
#include <Utils/YamlReader.h>
#include <Utils/StringUtils.h>
#include <yaml-cpp/yaml.h>

#include <Modules/BlueprintsStorage/BlueprintsStorage.h>
#include <Modules/Commutator/Commutator.h>
#include <Modules/Ship/Ship.h>
#include <Network/ProtobufChannel.h>
#include <Network/UdpSocket.h>
#include <Network/SessionMux.h>
#include <Modules/SystemClock/SystemClock.h>
#include <Network/UdpSocket.h>
#include <Network/SessionMux.h>
#include <Blueprints/Ships/ShipBlueprint.h>

namespace world
{

Player::Player(std::string&& sLogin,
               blueprints::BlueprintsLibrary&& blueprints)
  : m_sLogin(std::move(sLogin)),
    m_pSessionMux(std::make_shared<network::SessionMux>()),
    m_pRootCommutator(std::make_shared<modules::Commutator>(m_pSessionMux)),
    m_blueprints(std::move(blueprints))
{
  m_linker.link(m_pSessionMux, m_pRootCommutator);
}

PlayerPtr Player::load(
    std::string sLogin,
    blueprints::BlueprintsLibrary blueprints,
    YAML::Node const& state)
{
  // Can't use 'std::make_shared' here since Player's constructor is private
  PlayerPtr pPlayer =
      std::shared_ptr<Player>(
        new Player(std::move(sLogin), std::move(blueprints)));

  if (!utils::YamlReader(state).read("password", pPlayer->m_sPassword)) {
    assert(!"Password is not specified");
    return PlayerPtr();
  }

  pPlayer->m_pSystemClock =
      std::make_shared<modules::SystemClock>("SystemClock", pPlayer);
  pPlayer->m_pBlueprintsExplorer =
      std::make_shared<modules::BlueprintsStorage>(pPlayer);

  pPlayer->m_linker.attachModule(
    pPlayer->m_pRootCommutator, pPlayer->m_pSystemClock);
  pPlayer->m_linker.attachModule(
    pPlayer->m_pRootCommutator, pPlayer->m_pBlueprintsExplorer);

  YAML::Node const& shipsState = state["ships"];
  if (!shipsState.IsDefined()) {
    // Player has no ships (proval!)
    return pPlayer;
  }

  for (auto const& kv : shipsState) {
    std::string sShipTypeAndName = kv.first.as<std::string>();
    std::string sShipName;
    std::string sShipType;
    utils::StringUtils::split('/', sShipTypeAndName, sShipType, sShipName);
    assert(!sShipName.empty() && !sShipType.empty());

    blueprints::BaseBlueprintPtr pShipBlueprint =
        pPlayer->m_blueprints.getBlueprint(
          blueprints::BlueprintName("Ship", sShipType));
    assert(pShipBlueprint);
    if (!pShipBlueprint)
      return PlayerPtr();

    modules::ShipPtr pShip =
        std::static_pointer_cast<modules::Ship>(
          pShipBlueprint->build(std::move(sShipName), pPlayer));
    assert(pShip);
    if (!pShip) {
      return PlayerPtr();
    }

    if (!pShip->loadState(kv.second)) {
      assert("Failed to load ship" == nullptr);
      return PlayerPtr();
    }
    pPlayer->m_pRootCommutator->attachModule(std::move(pShip));
  }

  return pPlayer;
}

PlayerPtr Player::makeDummy(std::string sLogin)
{
  // Can't use 'std::make_shared' here since Player's constructor is private
  return std::shared_ptr<Player>(
        new Player(std::move(sLogin), blueprints::BlueprintsLibrary()));
}

uint32_t Player::onNewConnection(uint32_t nConnectionId)
{
  return m_pSessionMux->addConnection(nConnectionId, m_pRootCommutator);
}

void Player::attachToUdpSocket(network::UdpSocketPtr pSocket)
{
  m_pUdpChannel = pSocket;
  if (!m_pProtobufChannel) {
    m_pProtobufChannel = std::make_shared<network::PlayerChannel>();
    m_linker.link(m_pProtobufChannel, m_pSessionMux->asTerminal());
  }
  m_linker.link(m_pUdpChannel, m_pProtobufChannel);
}

void Player::onMessageReceived(uint32_t nSessionId, spex::Message const& message)
{
  if (message.choice_case() == spex::Message::kRootSession) {
    switch(message.root_session().choice_case()) {
      case spex::IRootSession::kNewCommutatorSession: {
        open_commutator_session(nSessionId);
        return;
      }
      default: {
        return;
      }
    }
  }
}

void Player::open_commutator_session(uint32_t nSessionId)
{
  uint32_t nChildSessionId = 0;
  {
    // Why do we need mutex here? If two messages are sent in different UDP
    // connections, they can be handled in different threads. Since Player
    // handles messages immediatelly (it doesn't have any buffers), a races
    // are possible here.
    std::lock_guard<utils::Mutex> guard(m_mutex);
    nChildSessionId = m_pSessionMux->createSession(
      nSessionId, m_pRootCommutator);
  }

  spex::Message message;
  spex::IRootSession* pBody = message.mutable_root_session();
  pBody->set_commutator_session(nChildSessionId);
  m_pSessionMux->asChannel()->send(nSessionId, std::move(message));
}

} // namespace world
