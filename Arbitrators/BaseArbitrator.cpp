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
    default:
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

} // namespace arbitrator
