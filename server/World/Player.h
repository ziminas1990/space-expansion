#pragma once

#include <memory>

#include <Utils/YamlForwardDeclarations.h>
#include <Network/ProtobufChannel.h>
#include <Modules/Commutator/Commutator.h>
#include <Ships/Ship.h>
#include <Modules/BlueprintsStorage/BlueprintsStorage.h>
#include <Modules/SystemClock/SystemClock.h>
#include <Blueprints/BlueprintsLibrary.h>

namespace world
{

class Player;
using PlayerPtr = std::shared_ptr<Player>;

class Player
{
  Player(std::string&& sLogin,
         blueprints::BlueprintsLibrary&& blueprints);

public:

  static PlayerPtr load(std::string sLogin, blueprints::BlueprintsLibrary blueprints,
                        YAML::Node const& state);
  ~Player();

  void attachToChannel(network::PlayerChannelPtr pChannel);

  std::string const& getLogin()    const { return m_sLogin; }
  std::string const& getPassword() const { return m_sPassword; }

  blueprints::BlueprintsLibrary&       getBlueprints()       { return m_blueprints; }
  blueprints::BlueprintsLibrary const& getBlueprints() const { return m_blueprints; }

  modules::CommutatorPtr getCommutator() const { return m_pEntryPoint; }

private:
  std::string const m_sLogin;
  std::string       m_sPassword;

  network::PlayerChannelPtr     m_pChannel;
  modules::CommutatorPtr        m_pEntryPoint;
  modules::SystemClockPtr       m_pSystemClock;
  modules::BlueprintsStoragePtr m_pBlueprintsExplorer;

  blueprints::BlueprintsLibrary m_blueprints;
    // Every player has it's own set of blueprint, that can be improoved during the game
    // At the start, all players have the same blueprints library

  std::vector<ships::ShipPtr>   m_ships;
};

} // namespace world
