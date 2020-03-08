#pragma once

#include <string>
#include <stdint.h>
#include <vector>
#include <atomic>
#include <memory>

#include <Conveyor/IAbstractLogic.h>
#include <World/PlayersStorage.h>

namespace arbitrator {

struct Leaderboard {
  struct Record {
    Record(std::string sLogin, uint32_t nScore)
      :  m_sLogin(std::move(sLogin)), m_nScore(nScore)
    {}

    bool operator<(Record const& other) const { return m_nScore < other.m_nScore; }

    std::string m_sLogin;
    uint32_t    m_nScore;
  };

  void sort();

  std::vector<Record> m_table;
};

// Implements base for all arbitrators.
// Each arbitrator (subclass) must implement:
// 1. `score()` - function, that calculates and returns score for some player
// 2. `loadConfiguation()` - reading arbitrator configuration
class BaseArbitrator : public conveyor::IAbstractLogic
{
  enum Stage {
    eScoring = 0,
    eCheckIfGameOver,
    eTotalStages
  };

public:
  BaseArbitrator(world::PlayerStoragePtr pPlayersStorage,
                 size_t nCooldownTime = 200 * 1000);

  virtual bool loadConfiguation(YAML::Node const& data);

  void addPlayer(std::string sLogin, double score = 0) {
    m_board.m_table.emplace_back(std::move(sLogin), score);
  }

  uint32_t getTargetScore() const { return m_nTargetScore; }
  void     changeTargetScore(uint32_t nTargetScore) { m_nTargetScore = nTargetScore; }

  // overrides from IAbstractLogic interface
  uint16_t getStagesCount() override { return eTotalStages; }
  bool     prephareStage(uint16_t nStageId) override;
  void     proceedStage(uint16_t nStageId, uint32_t nIntervalUs) override;
  size_t   getCooldownTimeUs() const override { return m_nCooldownTime; }

protected:
  virtual uint32_t score(world::PlayerPtr pPlayer) = 0;
  // This function should calculate some score for the specified 'pPlayer'.
  // When the player's score reaches 'm_nTargetScore', the game will be finished.
  // NOTE: This function will be called concurrently, so it must be thread safe.

  void onGameOver();

private:
  Leaderboard m_board;

  world::PlayerStoragePtr m_pPlayersStorage;
  uint32_t                m_nTargetScore;
  size_t                  m_nCooldownTime;

  std::atomic<size_t> m_nNextId;
};

using BaseArbitratorPtr = std::shared_ptr<BaseArbitrator>;

} // namespace arbitrator
