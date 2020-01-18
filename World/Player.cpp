#include "Player.h"
#include <Utils/YamlReader.h>
#include <Utils/StringUtils.h>
#include <yaml-cpp/yaml.h>

#include <Blueprints/Ships/ShipBlueprint.h>

namespace world
{

Player::Player(std::string sLogin,
               modules::BlueprintsLibrary avaliableModulesBlueprints,
               ships::ShipBlueprintsLibrary shipsBlueprints)
  : m_sLogin(std::move(sLogin)), m_pEntryPoint(std::make_shared<modules::Commutator>()),
    m_modulesBlueprints(std::move(avaliableModulesBlueprints)),
    m_shipsBlueprints(std::move(shipsBlueprints))
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

bool Player::loadState(YAML::Node const& data)
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
    std::string sShipName = kv.first.as<std::string>();

    ships::ShipBlueprintPtr pShipBlueprint = m_shipsBlueprints.getBlueprint(sShipName);
    assert(pShipBlueprint);
    if (!pShipBlueprint)
      return false;
    ships::ShipPtr pShip = pShipBlueprint->build(std::move(sShipName),
                                                 m_modulesBlueprints);
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
