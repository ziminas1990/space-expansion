#include "Player.h"
#include <Utils/YamlReader.h>
#include <Utils/StringUtils.h>
#include <yaml-cpp/yaml.h>

#include <Blueprints/Ships/ShipBlueprint.h>

namespace world
{

Player::Player(std::string sLogin, modules::BlueprintsLibrary blueprints)
  : m_sLogin(std::move(sLogin)),
    m_pEntryPoint(std::make_shared<modules::Commutator>()),
    m_pBlueprintsExplorer(std::make_shared<modules::BlueprintsStorage>()),
    m_blueprints(std::move(blueprints))
{
  m_pBlueprintsExplorer->attachToLibrary(&m_blueprints);
  m_pBlueprintsExplorer->attachToChannel(m_pEntryPoint);
  m_pEntryPoint->attachModule(m_pBlueprintsExplorer);
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
}

void Player::attachToChannel(network::ProtobufChannelPtr pChannel)
{
  m_pChannel = std::move(pChannel);
  m_pEntryPoint->attachToChannel(m_pChannel);
  m_pChannel->attachToTerminal(m_pEntryPoint);
}

bool Player::loadState(YAML::Node const& data)
{
  if (!utils::YamlReader(data).read("password", m_sPassword)) {
    assert(false);
    return false;
  }

  YAML::Node const& shipsState = data["ships"];
  if (!shipsState.IsDefined()) {
    // Player has no ships (looooser!)
    return true;
  }

  for (auto const& kv : shipsState) {
    std::string sShipTypeAndName = kv.first.as<std::string>();
    std::string sShipName;
    std::string sShipType;
    utils::StringUtils::split('/', sShipTypeAndName, sShipType, sShipName);
    assert(!sShipName.empty() && !sShipType.empty());

    modules::BaseBlueprintPtr pShipBlueprint =
        m_blueprints.getBlueprint(modules::BlueprintName("Ship", sShipType));
    assert(pShipBlueprint);
    if (!pShipBlueprint)
      return false;

    ships::ShipPtr pShip =
        std::dynamic_pointer_cast<ships::Ship>(
          pShipBlueprint->build(std::move(sShipName), m_blueprints));
    assert(pShip);
    if (!pShip)
      return false;

    if (!pShip->loadState(kv.second)) {
      assert(false);
      return false;
    }
    pShip->attachToChannel(m_pEntryPoint);
    m_pEntryPoint->attachModule(std::move(pShip));
  }
  return true;
}



} // namespace world
