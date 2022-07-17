#include <Autotests/Modules/ModulesTestFixture.h>

#include <Modules/Engine/Engine.h>
#include <Autotests/Modules/Helper.h>

namespace autotests {

class CommutatorTests : public ModulesTestFixture
{};

TEST_F(CommutatorTests, Breath)
{
  // Check that commutator works and we can spawn ships and modules and
  // attach to them

  ShipBinding ship = Helper::spawnShip(
    *this, m_pPlayer, geometry::Point(0, 0), Helper::ShipParams());

  const uint32_t nMaxThrust = 100000;
  EngineBinding engine = Helper::spawnEngine(
    ship, Helper::EngineParams().maxThrust(nMaxThrust));

  client::EngineSpecification spec;
  ASSERT_TRUE(engine->getSpecification(spec));
  ASSERT_EQ(nMaxThrust, spec.nMaxThrust);
}

TEST_F(CommutatorTests, CloseSession)
{
  // Check that if root session is closed, all other sessions will be closed
  // as well.

  ShipBinding ship = Helper::spawnShip(
    *this, m_pPlayer, geometry::Point(0, 0), Helper::ShipParams());

  const uint32_t nMaxThrust = 100000;
  EngineBinding engine = Helper::spawnEngine(
    ship, Helper::EngineParams().maxThrust(nMaxThrust));
}

}  // namespace autotests