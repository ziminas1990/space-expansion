#include <gtest/gtest.h>
#include <set>

#include <Utils/SimpleSubscriptionMixin.h>

class Inheriter: public Utils::SimpleSubscriptionMixin {};

bool checkForUpdates(Inheriter& obj,
                     uint64_t now,
                     std::set<uint32_t> expectedSessions)
{
  while (!expectedSessions.empty()) {
    uint32_t nSessionId;
    if (!obj.nextUpdate(nSessionId, now)) {
      return false;
    }
    auto it = expectedSessions.find(nSessionId);
    if (it == expectedSessions.end()) {
      return false;
    }
    expectedSessions.erase(it);
  }
  return true;
}

TEST(SimpleSubscriptionMixin, updates)
{
  Inheriter mixin;

  // Wil trigger at 3000, 5000, 7000
  mixin.addSubscription(2, 2000, 1000000);
  // Will trigger at 3000, 4000, 5000, 6000 and 7000
  mixin.addSubscription(1, 1000, 2000000);
  // Will trigger ad 4700, 5400, 6100, 6800, 7500
  mixin.addSubscription(5, 700, 4000000);

  struct Expectation {
    uint64_t m_timeMs;
    std::set<uint32_t> m_updates;
  };

  std::vector<Expectation> expectations = {
    {2000, {}},
    {2600, {}},
    {3200, {1, 2}},
    {3800, {}},
    {4300, {1}},
    {4900, {5}},
    {5400, {1, 2, 5}},
    {6000, {1}},
    {6600, {5}},
    {7200, {1, 2, 5}},
    {7800, {5}},
  };
  for (const Expectation& exp: expectations) {
    ASSERT_TRUE(checkForUpdates(
                  mixin, exp.m_timeMs * 1000, exp.m_updates))
        << "At " << exp.m_timeMs;
  }
}

TEST(SimpleSubscriptionMixin, remove)
{
  Inheriter mixin;

  // Wil trigger at 3000, 5000, 7000
  mixin.addSubscription(2, 2000, 1000000);
  // Will trigger at 3000, 4000, 5000, 6000 and 7000
  mixin.addSubscription(1, 1000, 2000000);
  // Will trigger ad 4700, 5400, 6100, 6800, 7500
  mixin.addSubscription(5, 700, 4000000);

  struct Expectation {
    uint64_t m_timeMs;
    std::set<uint32_t> m_updates;
  };

  {
    std::vector<Expectation> expectations = {
      {2000, {}},
      {2600, {}},
      {3200, {1, 2}},
      {3800, {}},
      {4300, {1}},
    };
    for (const Expectation& exp: expectations) {
      ASSERT_TRUE(checkForUpdates(
                    mixin, exp.m_timeMs * 1000, exp.m_updates))
          << "At " << exp.m_timeMs;
    }
  }

  mixin.removeSubscription(1);
  {
    std::vector<Expectation> expectations = {
      {5400, {2, 5}},
      {6000, {}},
      {6600, {5}},
    };
    for (const Expectation& exp: expectations) {
      ASSERT_TRUE(checkForUpdates(
                    mixin, exp.m_timeMs * 1000, exp.m_updates))
          << "At " << exp.m_timeMs;
    }
  }

  mixin.removeSubscription(5);
  {
    std::vector<Expectation> expectations = {
      {7200, {2}},
      {7800, {}},
      {8400, {}},
      {9000, {2}},
      {9600, {}},
    };
    for (const Expectation& exp: expectations) {
      ASSERT_TRUE(checkForUpdates(
                    mixin, exp.m_timeMs * 1000, exp.m_updates))
          << "At " << exp.m_timeMs;
    }
  }

  mixin.removeSubscription(2);
  {
    std::vector<Expectation> expectations = {
      {10200, {}},
      {10800, {}},
      {11400, {}},
      {12000, {}},
      {12600, {}},
    };
    for (const Expectation& exp: expectations) {
      ASSERT_TRUE(checkForUpdates(
                    mixin, exp.m_timeMs * 1000, exp.m_updates))
          << "At " << exp.m_timeMs;
    }
  }
}
