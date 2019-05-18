#include "FunctionalTestFixture.h"

#include "Scenarios.h"

namespace autotests
{

using ExploringShipsFunctionalTests = FunctionalTestFixture;

TEST_F(ExploringShipsFunctionalTests, GetShipsCount)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("admin", "admin")
        .expectSuccess());

  ASSERT_TRUE(m_pRootClientCommutator->getTotalSlots(7));
}


TEST_F(ExploringShipsFunctionalTests, GetShipsTypes)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("admin", "admin")
        .expectSuccess());

  // with cycle it's even more harder
  for(size_t i = 0; i < 1000; ++i) {
    ASSERT_TRUE(
          Scenarios::CheckAttachedModules(m_pRootClientCommutator)
          .hasModule("Ship/CommandCenter", 1)
          .hasModule("Ship/Miner", 2)
          .hasModule("Ship/Zond", 2)
          .hasModule("Ship/Corvet", 2)) << "on oteration #" << i;
  }
}



} // namespace autotests
