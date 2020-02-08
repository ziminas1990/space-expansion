#pragma once

#include <memory>

#include <Utils/YamlForwardDeclarations.h>
#include <Network/ProtobufChannel.h>
#include <Modules/Commutator/Commutator.h>
#include <Ships/Ship.h>
#include <Modules/BlueprintsStorage/BlueprintsStorage.h>
#include <Blueprints/BlueprintsLibrary.h>

namespace world
{

class Player
{
public:
  Player(std::string sLogin, modules::BlueprintsLibrary blueprints);
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

  modules::BlueprintsLibrary    m_blueprints;
    // Every player has it's own set of blueprint, that can be improoved during the game
    // At the start, all players have the same blueprints library

  std::vector<ships::ShipPtr>   m_ships;
};

using PlayerPtr = std::shared_ptr<Player>;

} // namespace world
