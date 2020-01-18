#include "PlayersStorage.h"
#include <Blueprints/Ships/ShipBlueprint.h>

#include <yaml-cpp/yaml.h>

namespace world {


bool PlayersStorage::loadState(
    YAML::Node const& data,
    modules::BlueprintsLibrary   const& avaliableModulesBlueprints,
    ships::ShipBlueprintsLibrary const& shipsBlueprints)
{
  for(auto const& kv : data) {
    std::string sLogin = kv.first.as<std::string>();
    assert(!sLogin.empty());
    if (sLogin.empty())
      return false;
    PlayerPtr pPlayer =
        std::make_shared<Player>(
          sLogin, avaliableModulesBlueprints, shipsBlueprints);
    if (!pPlayer->loadState(kv.second)) {
      assert(false);
      return false;
    }
    if (m_players.find(sLogin) != m_players.end()) {
      assert(false);
      return false;
    }
    m_players[sLogin] = pPlayer;
  }
  return true;
}

PlayerPtr PlayersStorage::getPlayer(std::string const& sLogin) const
{
  std::lock_guard<utils::Mutex> guard(m_Mutex);
  auto I = m_players.find(sLogin);
  return (I != m_players.end()) ? I->second : PlayerPtr();
}

} // namespace world
