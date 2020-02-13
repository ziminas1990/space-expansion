#include "Player.h"
#include <Utils/YamlReader.h>
#include <Utils/StringUtils.h>
#include <yaml-cpp/yaml.h>

#include <Blueprints/Ships/ShipBlueprint.h>

namespace world
{

Player::Player(std::string&& sLogin, modules::BlueprintsLibrary&& blueprints)
  : m_sLogin(std::move(sLogin)),
    m_blueprints(std::move(blueprints))
{}

PlayerPtr Player::load(std::string sLogin, modules::BlueprintsLibrary blueprints,
                       YAML::Node const& state)
{
  PlayerPtr pPlayer =
      std::shared_ptr<Player>(
        new Player(std::move(sLogin), std::move(blueprints)));

  if (!utils::YamlReader(state).read("password", pPlayer->m_sPassword)) {
    assert(false);
    return PlayerPtr();
  }

  pPlayer->m_pBlueprintsExplorer =
      std::make_shared<modules::BlueprintsStorage>(pPlayer);
  pPlayer->m_pEntryPoint = std::make_shared<modules::Commutator>();

  pPlayer->m_pBlueprintsExplorer->attachToChannel(pPlayer->m_pEntryPoint);
  pPlayer->m_pEntryPoint->attachModule(pPlayer->m_pBlueprintsExplorer);

  YAML::Node const& shipsState = state["ships"];
  if (!shipsState.IsDefined()) {
    // Player has no ships (looooser!)
    return pPlayer;
  }

  for (auto const& kv : shipsState) {
    std::string sShipTypeAndName = kv.first.as<std::string>();
    std::string sShipName;
    std::string sShipType;
    utils::StringUtils::split('/', sShipTypeAndName, sShipType, sShipName);
    assert(!sShipName.empty() && !sShipType.empty());

    modules::BaseBlueprintPtr pShipBlueprint =
        pPlayer->m_blueprints.getBlueprint(modules::BlueprintName("Ship", sShipType));
    assert(pShipBlueprint);
    if (!pShipBlueprint)
      return PlayerPtr();

    ships::ShipPtr pShip =
        std::dynamic_pointer_cast<ships::Ship>(
          pShipBlueprint->build(std::move(sShipName), pPlayer));
    assert(pShip);
    if (!pShip)
      return PlayerPtr();

    if (!pShip->loadState(kv.second)) {
      assert(false);
      return PlayerPtr();
    }
    pShip->attachToChannel(pPlayer->m_pEntryPoint);
    pPlayer->m_pEntryPoint->attachModule(std::move(pShip));
  }

  return pPlayer;
}

Player::~Player()
{
  if (m_pChannel) {
    m_pChannel->detachFromTerminal();
    m_pChannel->detachFromChannel();
  }
  m_pEntryPoint->detachFromChannel();
  m_pEntryPoint->detachFromTerminal();
  m_pEntryPoint->detachFromModules();
  m_pBlueprintsExplorer->detachFromChannel();
}

void Player::attachToChannel(network::ProtobufChannelPtr pChannel)
{
  m_pChannel = std::move(pChannel);
  m_pEntryPoint->attachToChannel(m_pChannel);
  m_pChannel->attachToTerminal(m_pEntryPoint);
}

} // namespace world
