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
        .expectSuccess()
        .run());

  ASSERT_TRUE(m_pRootClientCommutator->sendGetTotalSlots(7));
}

} // namespace autotests
