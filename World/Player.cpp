#include "Player.h"
#include <Utils/YamlReader.h>
#include <Utils/StringUtils.h>
#include <yaml-cpp/yaml.h>

#include <Blueprints/Ships/ShipBlueprint.h>

namespace world
{

Player::Player(std::string sLogin)
  : m_sLogin(std::move(sLogin)), m_pEntryPoint(std::make_shared<modules::Commutator>())
{}

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

bool Player::loadState(YAML::Node const& data,
                       blueprints::BlueprintsStoragePtr pBlueprints)
{
  if (!utils::YamlReader(data).read("password", m_sPassword)) {
    assert(false);
    return false;
  }

  YAML::Node const& shipsState = data["ships"];
  if (!shipsState.IsDefined()) {
    assert(false);
    return false;
  }

  for (auto const& kv : shipsState) {
    std::string sShipType;
    std::string sShipName;
    utils::StringUtils::split('/', kv.first.as<std::string>(), sShipType, sShipName);

    ships::ShipBlueprintConstPtr pShipBlueprint = pBlueprints->getBlueprint(sShipType);
    assert(pShipBlueprint);
    if (!pShipBlueprint)
      return false;
    ships::ShipPtr pShip = pShipBlueprint->build(std::move(sShipName));
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
