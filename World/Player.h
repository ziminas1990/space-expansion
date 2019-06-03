#pragma once

#include <memory>

#include <Utils/YamlForwardDeclarations.h>
#include <Network/ProtobufChannel.h>
#include <Modules/Commutator/Commutator.h>
#include <Ships/Ship.h>
#include <Blueprints/BlueprintsStorage.h>

namespace world
{

class Player
{
public:
  Player(std::string sLogin);
  ~Player();

  bool loadState(YAML::Node const& data, blueprints::BlueprintsStoragePtr pBlueprints);

  void attachToChannel(network::ProtobufChannelPtr pChannel);

  std::string const& getLogin()    const { return m_sLogin; }
  std::string const& getPassword() const { return m_sPassword; }

private:
  std::string const m_sLogin;
  std::string       m_sPassword;

  network::ProtobufChannelPtr   m_pChannel;
  modules::CommutatorPtr        m_pEntryPoint;
  std::vector<ships::ShipPtr>   m_ships;
};

using PlayerPtr = std::shared_ptr<Player>;

} // namespace world
