#pragma once

#include <memory>

#include <Utils/YamlForwardDeclarations.h>
#include <Network/Fwd.h>
#include <Network/ProtobufChannel.h>
#include <Modules/Fwd.h>
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
  static PlayerPtr load(std::string sLogin,
                        blueprints::BlueprintsLibrary blueprints,
                        YAML::Node const& state);

  static PlayerPtr makeDummy(std::string sLogin);
  // Create non initialized Player object (may be used in tests purposes)

  ~Player();

  network::UdpSocketPtr getUdpSocket() const { return m_pUdpChannel; }
  void attachToUdpSocket(network::UdpSocketPtr pSocket);

  network::SessionMuxPtr getSessionMux() const { return m_pSesionMux; }

  // Register a new connection and return a root sessionId
  uint32_t onNewConnection(uint32_t nConnectionId);

  std::string const& getLogin()    const { return m_sLogin; }
  std::string const& getPassword() const { return m_sPassword; }

  blueprints::BlueprintsLibrary&       getBlueprints()       { return m_blueprints; }
  blueprints::BlueprintsLibrary const& getBlueprints() const { return m_blueprints; }

  modules::CommutatorPtr getCommutator() const { return m_pEntryPoint; }

private:
  std::string const m_sLogin;
  std::string       m_sPassword;

  network::UdpSocketPtr         m_pUdpChannel;
  network::PlayerChannelPtr     m_pProtobufChannel;
  network::SessionMuxPtr        m_pSesionMux;
  modules::CommutatorPtr        m_pEntryPoint;
  modules::SystemClockPtr       m_pSystemClock;
  modules::BlueprintsStoragePtr m_pBlueprintsExplorer;

  blueprints::BlueprintsLibrary m_blueprints;
    // Every player has it's own set of blueprint, that can be improoved during 
    // the game. At the beginning, all players have the same blueprints library

  std::vector<modules::ShipPtr> m_ships;
};

} // namespace world
