#pragma once

#include <memory>

#include <Utils/YamlForwardDeclarations.h>
#include <Network/Interfaces.h>
#include <Network/Fwd.h>
#include <Network/ProtobufChannel.h>
#include <Modules/Fwd.h>
#include <Blueprints/BlueprintsLibrary.h>
#include <Utils/Linker.h>
#include <Utils/Mutex.h>

namespace world
{

class Player;
using PlayerPtr = std::shared_ptr<Player>;

class RootSession;
using RootSessionPtr = std::shared_ptr<RootSession>;

// Player aggregates all components, related to some specific player.
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

  network::UdpSocketPtr getUdpSocket() const { return m_pUdpChannel; }
  void attachToUdpSocket(network::UdpSocketPtr pSocket);

  network::SessionMuxPtr getSessionMux() const { return m_pSessionMux; }

  // Register a new connection and return a root sessionId
  uint32_t onNewConnection(uint32_t nConnectionId);

  std::string const& getLogin()    const { return m_sLogin; }
  std::string const& getPassword() const { return m_sPassword; }

  blueprints::BlueprintsLibrary&       getBlueprints()       { return m_blueprints; }
  blueprints::BlueprintsLibrary const& getBlueprints() const { return m_blueprints; }

  modules::CommutatorPtr getCommutator() const { return m_pRootCommutator; }

private:
  // Handle 'openCommutatorSession' reqiest, received in session, specified
  // by 'nSessionId'.
  void open_commutator_session(uint32_t nSessionId);

private:
  std::string  m_sLogin;
  std::string  m_sPassword;

  network::UdpSocketPtr         m_pUdpChannel;
  network::PlayerChannelPtr     m_pProtobufChannel;
  network::SessionMuxPtr        m_pSessionMux;
  modules::CommutatorPtr        m_pRootCommutator;
  modules::SystemClockPtr       m_pSystemClock;
  modules::BlueprintsStoragePtr m_pBlueprintsExplorer;
  RootSessionPtr                m_pRootSession;

  blueprints::BlueprintsLibrary m_blueprints;
    // Every player has it's own set of blueprints, that can be improved during
    // the game. At the beginning, all players have the same blueprints library

  std::vector<modules::ShipPtr> m_ships;

  // Linker is placed to the end to be destroyed first (and destroy all links)
  utils::Linker m_linker;
};

} // namespace world
