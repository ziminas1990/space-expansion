#include "Player.h"
#include <Utils/YamlReader.h>
#include <Utils/StringUtils.h>
#include <yaml-cpp/yaml.h>

#include <Blueprints/Ships/ShipBlueprint.h>

namespace world
{

Player::Player(std::string&& sLogin,
               blueprints::BlueprintsLibrary&& blueprints)
  : m_sLogin(std::move(sLogin)),
    m_blueprints(std::move(blueprints))
{}

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
    assert(false);
    return PlayerPtr();
  }

  pPlayer->m_pSesionMux  = std::make_shared<network::SessionMux>();
  pPlayer->m_pEntryPoint = std::make_shared<modules::Commutator>(
    pPlayer->m_pSesionMux
  );

  pPlayer->m_pEntryPoint->attachToChannel(pPlayer->m_pSesionMux->asChannel());

  pPlayer->m_pSystemClock =
      std::make_shared<modules::SystemClock>("SystemClock", pPlayer);
  pPlayer->m_pEntryPoint->attachModule(pPlayer->m_pSystemClock);

  pPlayer->m_pBlueprintsExplorer =
      std::make_shared<modules::BlueprintsStorage>(pPlayer);
  pPlayer->m_pEntryPoint->attachModule(pPlayer->m_pBlueprintsExplorer);

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

    ships::ShipPtr pShip =
        std::static_pointer_cast<ships::Ship>(
          pShipBlueprint->build(std::move(sShipName), pPlayer));
    assert(pShip);
    if (!pShip)
      return PlayerPtr();

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

Player::~Player()
{
  if (m_pUdpChannel) {
    m_pUdpChannel->detachFromTerminal();
  }
  if (m_pProtobufChannel) {
    m_pProtobufChannel->detachFromTerminal();
    m_pProtobufChannel->detachFromChannel();
  }
  if (m_pSesionMux) {
    m_pSesionMux->detach();
  }
  if (m_pEntryPoint) {
    m_pEntryPoint->detachFromChannel();
    m_pEntryPoint->detachFromModules();
  }
  if (m_pBlueprintsExplorer) {
    m_pBlueprintsExplorer->detachFromChannel();
  }
}

void Player::attachToUdpSocket(network::UdpSocketPtr pSocket)
{
  m_pUdpChannel = pSocket;
  if (!m_pProtobufChannel) {
    m_pProtobufChannel = std::make_shared<network::PlayerChannel>();
    m_pEntryPoint->attachToChannel(m_pProtobufChannel);
    m_pProtobufChannel->attachToTerminal(m_pEntryPoint);
  }
  m_pProtobufChannel->attachToChannel(m_pUdpChannel);
  m_pUdpChannel->attachToTerminal(m_pProtobufChannel);
}

} // namespace world
