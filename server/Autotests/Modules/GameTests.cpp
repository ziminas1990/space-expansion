#include <gtest/gtest.h>

#include <initializer_list>
#include <utility>

#include <Modules/Game/Game.h>
#include <Autotests/ClientSDK/Modules/ClientGame.h>
#include <Protocol.pb.h>

#include "ModulesTestFixture.h"
#include "Helper.h"

namespace autotests {

class GameTests : public ModulesTestFixture
{
protected:
  spex::IGame::GameOver makeReport(
      std::initializer_list<std::pair<const char*, uint32_t>> leaders)
  {
    spex::IGame::GameOver report;
    for (const auto& leader : leaders) {
      spex::IGame::Score* pScore = report.add_leaders();
      pScore->set_player(leader.first);
      pScore->set_score(leader.second);
    }
    return report;
  }
};

TEST_F(GameTests, ListedOnRootCommutator)
{
  // 1. attach game to the player's root commutator
  Helper::createGameModule(*this);

  // 2. connect and open a commutator session
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  // 3. game is listed among attached modules
  client::ModulesList attached;
  ASSERT_TRUE(pCommutator->getAttachedModulesList(attached));
  ASSERT_EQ(1u, attached.size());
  EXPECT_EQ("Game", attached.front().sModuleType);
  EXPECT_EQ("Game", attached.front().sModuleName);
}

TEST_F(GameTests, MonitorReceivesGameOver)
{
  // 1. attach game and open a tunnel to it
  Helper::createGameModule(*this);
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  client::GamePtr pGame = Helper::getGame(pCommutator);
  ASSERT_TRUE(pGame);

  // 2. send monitor
  ASSERT_TRUE(pGame->monitor());
  justWait(20);

  // 3. server notifies monitors when the match ends
  m_pPlayer->getGame()->notifyGameOver(makeReport({{"Player-1", 1000}}));

  // 4. the same session receives the end-of-game result
  spex::IGame::GameOver report;
  ASSERT_TRUE(pGame->waitGameOverReport(report));
  ASSERT_EQ(1, report.leaders_size());
  EXPECT_EQ("Player-1", report.leaders(0).player());
  EXPECT_EQ(1000u, report.leaders(0).score());
}

TEST_F(GameTests, ClosingMonitorStopsUpdatesForThatClient)
{
  // 1. attach game and open two monitoring sessions
  Helper::createGameModule(*this);
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  client::GamePtr pFirst = Helper::getGame(pCommutator);
  client::GamePtr pSecond = Helper::getGame(pCommutator);
  ASSERT_TRUE(pFirst);
  ASSERT_TRUE(pSecond);
  ASSERT_TRUE(pFirst->monitor());
  ASSERT_TRUE(pSecond->monitor());
  justWait(20);

  // 2. close the first monitoring session
  ASSERT_TRUE(pFirst->disconnect());
  justWait(20);

  // 3. remaining monitor still receives the end-of-game result
  m_pPlayer->getGame()->notifyGameOver(makeReport({{"Player-1", 42}}));

  spex::IGame::GameOver report;
  ASSERT_TRUE(pSecond->waitGameOverReport(report));
  ASSERT_EQ(1, report.leaders_size());
  EXPECT_EQ(42u, report.leaders(0).score());

  // 4. the closed session does not receive updates
  spex::IGame::GameOver ignored;
  ASSERT_FALSE(pFirst->waitGameOverReport(ignored, 50));
}

TEST_F(GameTests, LateMonitorReceivesStoredGameOver)
{
  // 1. attach game and finish the match before anyone monitors
  Helper::createGameModule(*this);
  m_pPlayer->getGame()->notifyGameOver(makeReport({{"Winner", 99}}));

  // 2. connect and start monitoring
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  client::GamePtr pGame = Helper::getGame(pCommutator);
  ASSERT_TRUE(pGame);
  ASSERT_TRUE(pGame->monitor());

  // 3. the stored end-of-game result is sent immediately
  spex::IGame::GameOver report;
  ASSERT_TRUE(pGame->waitGameOverReport(report));
  ASSERT_EQ(1, report.leaders_size());
  EXPECT_EQ("Winner", report.leaders(0).player());
  EXPECT_EQ(99u, report.leaders(0).score());
}

} // namespace autotests
