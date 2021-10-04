#include <gtest/gtest.h>
#include <set>

#include <Utils/SubscriptionsBox.h>

struct Expectation {
  Expectation(uint32_t atMs)
    : m_timeMs(atMs)
  {}

  uint32_t m_timeMs;
  std::set<uint32_t> m_updates;
};

struct Expectations {
  Expectations(uint32_t startMs, uint32_t endMs, uint32_t stepMs = 100)
  {
    for (uint32_t t = startMs; t < endMs; t += stepMs) {
      m_expectations.push_back(Expectation(t));
    }
  }

  void addSubscription(uint32_t sessionId, uint32_t startMs, uint32_t periodMs)
  {
    uint32_t nextUpdateMs = startMs + periodMs;
    for (Expectation& expectation: m_expectations) {
      if (expectation.m_timeMs >= nextUpdateMs) {
        expectation.m_updates.insert(sessionId);
        nextUpdateMs += periodMs;
      }
    }
  }

  void removeSubscription(uint32_t sessionId, uint32_t atMs)
  {
    for (Expectation& expectation: m_expectations) {
      if (expectation.m_timeMs >= atMs) {
        expectation.m_updates.erase(sessionId);
      }
    }
  }

  const Expectation operator[](size_t index) const {
    return m_expectations[index];
  }

  std::vector<Expectation> m_expectations;
};

bool checkForUpdates(utils::SubscriptionsBox& box,
                     uint32_t nowMs,
                     std::set<uint32_t> expectedSessions)
{
  while (!expectedSessions.empty()) {
    uint32_t nSessionId;
    if (!box.nextUpdate(nSessionId, nowMs * 1000)) {
      return false;
    }
    auto it = expectedSessions.find(nSessionId);
    if (it == expectedSessions.end()) {
      return false;
    }
    expectedSessions.erase(it);
  }
  uint32_t nSessionId;
  // CHeck that no more updates are expected
  return box.nextUpdate(nSessionId, nowMs * 1000) == false;
}

TEST(SimpleSubscriptionMixin, updates)
{
  utils::SubscriptionsBox box;

  Expectations expectations(0, 20000, 200);

  box.add(2, 2112, 1000000);
  expectations.addSubscription(2, 1000, 2112);
  box.add(1, 1847, 2000000);
  expectations.addSubscription(1, 2000, 1847);
  box.add(5, 711, 4000000);
  expectations.addSubscription(5, 4000, 711);

  for (const Expectation& expectation: expectations.m_expectations) {
    ASSERT_TRUE(checkForUpdates(
                  box,
                  expectation.m_timeMs,
                  expectation.m_updates))
        << "At " << expectation.m_timeMs;
  }
}

TEST(SimpleSubscriptionMixin, remove)
{  
  utils::SubscriptionsBox box;

  Expectations expectations(0, 200000, 100);

  box.add(2, 3243, 0);
  expectations.addSubscription(2, 0, 3243);

  box.add(1, 1253, 0);
  expectations.addSubscription(1, 0, 1253);

  box.add(5, 713, 0);
  expectations.addSubscription(5, 0, 713);

  box.add(3, 1267, 0);
  expectations.addSubscription(3, 0, 1267);

  struct RemoveOp {
    uint32_t sessionId;
    uint32_t removeAtMs;
  };
  const RemoveOp removeOps[] = {
    {2, 20000}, {5, 35000}, {1, 50000}, {3, 90000}
  };

  for (const RemoveOp& op: removeOps) {
    expectations.removeSubscription(op.sessionId, op.removeAtMs);
  }

  size_t i = 0;
  for (const RemoveOp& op: removeOps) {
    while (expectations[i].m_timeMs < op.removeAtMs) {
      checkForUpdates(box, expectations[i].m_timeMs, expectations[i].m_updates);
      ++i;
    }
    box.remove(op.sessionId);
  }
}

TEST(SimpleSubscriptionMixin, collapse_expectations)
{
  utils::SubscriptionsBox box;

  Expectations expectations(0, 10000, 500);

  box.add(1, 100, 0);
  expectations.addSubscription(1, 0, 100);
  box.add(2, 200, 0);
  expectations.addSubscription(2, 0, 200);

  for (const Expectation& expectation: expectations.m_expectations) {
    ASSERT_TRUE(checkForUpdates(
                  box,
                  expectation.m_timeMs,
                  expectation.m_updates))
        << "At " << expectation.m_timeMs;
  }
}
