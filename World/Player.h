#pragma once

#include <memory>

#include <Utils/YamlForwardDeclarations.h>
#include <Network/ProtobufChannel.h>
#include <Modules/Commutator/Commutator.h>
#include <Ships/Ship.h>
#include <Modules/BlueprintsStorage/BlueprintsStorage.h>
#include <Blueprints/Modules/BlueprintsLibrary.h>
#include <Blueprints/Ships/ShipBlueprintsLibrary.h>

namespace world
{

class Player
{
public:
  Player(std::string                  sLogin,
         modules::BlueprintsLibrary   avaliableModulesBlueprints,
         ships::ShipBlueprintsLibrary shipsBlueprints);
  ~Player();

  bool loadState(YAML::Node const& data);

  void attachToChannel(network::ProtobufChannelPtr pChannel);

  std::string const& getLogin()    const { return m_sLogin; }
  std::string const& getPassword() const { return m_sPassword; }

private:
  std::string const m_sLogin;
  std::string       m_sPassword;

  network::ProtobufChannelPtr   m_pChannel;
  modules::CommutatorPtr        m_pEntryPoint;
  modules::BlueprintsStoragePtr m_pBlueprintsExplorer;

  modules::BlueprintsLibrary    m_modulesBlueprints;
    // Every player has it's own set of blueprint, that can be improoved during the game
    // At the start, all players have the same blueprints library

  ships::ShipBlueprintsLibrary  m_shipsBlueprints;
    // Every player has it's own set of ship's blueprints, that player can build.
    // At the start, all players have the same set of ship's blueprints

  std::vector<ships::ShipPtr>   m_ships;
};

using PlayerPtr = std::shared_ptr<Player>;

} // namespace world
