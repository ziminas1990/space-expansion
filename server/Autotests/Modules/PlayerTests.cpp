#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <Autotests/Modules/Helper.h>
#include <Autotests/Modules/ModulesTestFixture.h>
#include <Modules/Commutator/Commutator.h>
#include <Modules/Ship/Ship.h>
#include <World/Player.h>

namespace autotests {

class PlayerTests : public ModulesTestFixture
{};

namespace {

modules::ShipPtr attachShip(const world::PlayerPtr& pPlayer,
                            const std::string& sName)
{
  auto pShip = std::make_shared<modules::Ship>(
      "Ship/Scout", sName, pPlayer, 1000, 10);
  pPlayer->onNewShip(pShip);
  return pShip;
}

} // namespace

TEST_F(PlayerTests, UniqueShipNamesFollowSuffixRule)
{
  struct Case {
    std::vector<std::string> occupied;
    std::string              requested;
    std::string              assigned;
  };

  const Case cases[] = {
      {{},                              "Scout",      "Scout"},
      {{"Scout"},                       "Scout",      "Scout-I"},
      {{"Scout", "Scout-I"},            "Scout",      "Scout-II"},
      {{"Scout-1"},                     "Scout",      "Scout"},
      {{"Scout-1"},                     "Scout-1",    "Scout-1-I"},
      {{"Sweet Home"},                  "Sweet Home", "Sweet Home I"},
      {{"Sweet Home", "Sweet Home I"},  "Sweet Home", "Sweet Home II"},
      {{"Drone #1"},                    "Drone #1",    "Drone #1 I"},
      {{"Scout\tHome"},                 "Scout\tHome", "Scout\tHome-I"},
      {{"Scout", "Scout-I", "Scout-II", "Scout-III"}, "Scout", "Scout-IV"},
  };

  // 1. for each suffix-table row, attach occupying ships then the requested ship
  for (const Case& row : cases) {
    SCOPED_TRACE(row.requested + " -> " + row.assigned);

    // 1.1 create a player and attach occupying ships
    world::PlayerPtr pPlayer = world::Player::makeDummy("tester");
    std::vector<modules::ShipPtr> occupying;
    occupying.reserve(row.occupied.size());
    for (const std::string& sName : row.occupied) {
      occupying.push_back(attachShip(pPlayer, sName));
      ASSERT_EQ(sName, occupying.back()->getModuleName());
    }

    // 1.2 attach the requested ship
    modules::ShipPtr pShip = attachShip(pPlayer, row.requested);

    // 1.3 check the assigned name and that occupying ships were not renamed
    EXPECT_EQ(row.assigned, pShip->getModuleName());
    for (size_t i = 0; i < occupying.size(); ++i) {
      EXPECT_EQ(row.occupied[i], occupying[i]->getModuleName());
    }
  }
}

TEST_F(PlayerTests, EmptyShipNameFallsBackToBlueprint)
{
  // 1. attach a ship with an empty name
  world::PlayerPtr pPlayer = world::Player::makeDummy("tester");
  auto pShip = std::make_shared<modules::Ship>(
      "Ship/Scout", std::string(), pPlayer, 1000, 10);
  ASSERT_NE(modules::Commutator::invalidSlot(), pPlayer->onNewShip(pShip));

  // 2. the blueprint name is used instead
  EXPECT_EQ("Ship/Scout", pShip->getModuleName());
}

TEST_F(PlayerTests, EmptyShipNameFallsBackThenGetsSuffix)
{
  // 1. occupy the blueprint name
  world::PlayerPtr pPlayer = world::Player::makeDummy("tester");
  modules::ShipPtr pOccupying = attachShip(pPlayer, "Ship/Scout");
  ASSERT_EQ("Ship/Scout", pOccupying->getModuleName());

  // 2. attach another ship with an empty name
  auto pShip = std::make_shared<modules::Ship>(
      "Ship/Scout", std::string(), pPlayer, 1000, 10);
  ASSERT_NE(modules::Commutator::invalidSlot(), pPlayer->onNewShip(pShip));

  // 3. the fallback name is uniquified
  EXPECT_EQ("Ship/Scout-I", pShip->getModuleName());
}

TEST_F(PlayerTests, SameNameIsAllowedForDifferentPlayers)
{
  // 1. attach a Scout to the first player
  world::PlayerPtr pFirst = world::Player::makeDummy("alice");
  modules::ShipPtr pAliceShip = attachShip(pFirst, "Scout");

  // 2. attach a Scout to the second player
  world::PlayerPtr pSecond = world::Player::makeDummy("bob");
  modules::ShipPtr pBobShip = attachShip(pSecond, "Scout");

  // 3. both keep the requested name
  EXPECT_EQ("Scout", pAliceShip->getModuleName());
  EXPECT_EQ("Scout", pBobShip->getModuleName());
}

TEST_F(PlayerTests, DestroyedShipNameCanBeReused)
{
  // 1. attach an occupying ship
  world::PlayerPtr pPlayer = world::Player::makeDummy("tester");
  modules::ShipPtr pOccupying = attachShip(pPlayer, "Scout");
  ASSERT_EQ("Scout", pOccupying->getModuleName());

  // 2. destroy the occupying ship
  pOccupying->onDoestroyed();

  // 3. attach another ship with the same requested name
  modules::ShipPtr pReplacement = attachShip(pPlayer, "Scout");

  // 4. the original name is free again
  EXPECT_EQ("Scout", pReplacement->getModuleName());
}

TEST_F(PlayerTests, ShipNameMayMatchNonShipModule)
{
  // 1. attach a Messanger module named Messanger
  Helper::createMessangerModule(*this);

  // 2. attach a ship with the same name
  modules::ShipPtr pShip = attachShip(m_pPlayer, "Messanger");

  // 3. the ship keeps the requested name
  EXPECT_EQ("Messanger", pShip->getModuleName());
}

} // namespace autotests
