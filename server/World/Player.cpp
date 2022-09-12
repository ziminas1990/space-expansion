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
    m_pSesionMux(std::make_shared<network::SessionMux>()),
    m_pEntryPoint(std::make_shared<modules::Commutator>(m_pSesionMux)),
    m_blueprints(std::move(blueprints))
{
  m_linker.link(m_pSesionMux, m_pEntryPoint);
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
    pPlayer->m_pEntryPoint, pPlayer->m_pSystemClock);
  pPlayer->m_linker.attachModule(
    pPlayer->m_pEntryPoint, pPlayer->m_pBlueprintsExplorer);

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
    pPlayer->m_pEntryPoint->attachModule(std::move(pShip));
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
  return m_pSesionMux->addConnection(nConnectionId, m_pEntryPoint);
}

uint32_t Player::createAdditionalSession(uint32_t nConnectionId)
{
  return m_pSesionMux->createSession(nConnectionId);
}

void Player::attachToUdpSocket(network::UdpSocketPtr pSocket)
{
  m_pUdpChannel = pSocket;
  if (!m_pProtobufChannel) {
    m_pProtobufChannel = std::make_shared<network::PlayerChannel>();
    m_linker.link(m_pProtobufChannel, m_pSesionMux->asTerminal());
  }
  m_linker.link(m_pUdpChannel, m_pProtobufChannel);
}

} // namespace world
