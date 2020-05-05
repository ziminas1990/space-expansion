#include "PlayersStorage.h"
#include <Blueprints/Ships/ShipBlueprint.h>

#include <assert.h>
#include <yaml-cpp/yaml.h>

namespace world {


bool PlayersStorage::loadState(YAML::Node const& data,
                               blueprints::BlueprintsLibrary const& blueprints)
{
  for(auto const& kv : data) {
    std::string sLogin = kv.first.as<std::string>();
    assert(!sLogin.empty());
    if (sLogin.empty())
      return false;
    PlayerPtr pPlayer = Player::load(sLogin, blueprints, kv.second);
    assert(pPlayer != nullptr);
    if (!pPlayer) {
      return false;
    }
    if (m_loginToPlayer.find(sLogin) != m_loginToPlayer.end()) {
      assert(false);
      return false;
    }
    m_loginToPlayer[sLogin] = m_players.size();
    m_players.push_back(std::move(pPlayer));
  }
  return true;
}

PlayerPtr PlayersStorage::getPlayer(std::string const& sLogin) const
{
  std::lock_guard<utils::Mutex> guard(m_Mutex);
  auto I = m_loginToPlayer.find(sLogin);
  if (I == m_loginToPlayer.end())
    return nullptr;
  size_t nPlayerId = I->second;
  assert(nPlayerId < m_players.size());
  return m_players[nPlayerId];
}

} // namespace world
