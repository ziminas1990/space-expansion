#include "BaseArbitrator.h"

#include <assert.h>

namespace arbitrator {

void Leaderboard::sort()
{
  size_t nBoardLength = m_table.size();
  // Insertion sort is the best option here
  for (size_t i = 1; i < nBoardLength; ++i) {
    for (size_t j = i; j > 0; --j) {
      if (m_table[j] < m_table[j - 1]) {
        std::swap(m_table[j], m_table[j - 1]);
      } else {
        break;
      }
    }
  }
}

BaseArbitrator::BaseArbitrator(world::PlayerStoragePtr pPlayersStorage,
                               double nTargetScore, size_t nCooldownTime)
  : m_pPlayersStorage(pPlayersStorage),
    m_nTargetScore(nTargetScore),
    m_nCooldownTime(nCooldownTime)
{
}

bool BaseArbitrator::prephareStage(uint16_t nStageId)
{
  switch (nStageId) {
    case eScoring:
      m_nNextId.store(0);
      return true;
    case eCheckIfGameOver:
      m_board.sort();
      if (m_board.m_table.front().m_nScore >= m_nTargetScore) {
        onGameOver();
      }
      m_nCooldownTime = 10 * 1000 * 1000; // 10 sec
      return false;
    default:
      assert("Unexpected stage" == nullptr);
      return false;
  }
}

void BaseArbitrator::proceedStage(uint16_t nStageId, uint32_t)
{
  assert(nStageId == eScoring);
  size_t id = m_nNextId.fetch_add(1);
  while (id < m_board.m_table.size()) {
    Leaderboard::Record& record = m_board.m_table[id];
    world::PlayerPtr pPlayer = m_pPlayersStorage->getPlayer(record.m_sLogin);
    record.m_nScore = score(pPlayer);
    id = m_nNextId.fetch_add(1);
  }
}

void BaseArbitrator::onGameOver()
{
  spex::Message message;
  spex::IGame::GameOver* pGameOver = message.mutable_game()->mutable_game_over_report();

  for (Leaderboard::Record const& record: m_board.m_table) {
    spex::IGame::Score* pScore = pGameOver->mutable_leaders()->Add();
    pScore->set_player(record.m_sLogin);
    pScore->set_score(record.m_nScore);
  }

  for (world::PlayerPtr pPlayer: m_pPlayersStorage->getAllPlayers()) {
    pPlayer->getCommutator()->broadcast(message);
  }
}

} // namespace arbitrator
