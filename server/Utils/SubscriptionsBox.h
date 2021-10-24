#pragma once

#include <stdint.h>
#include <vector>

namespace utils {

class SubscriptionsBox
{
public:
  void add(uint32_t nSessionId, uint32_t nPeriodMs, uint64_t now);
  // Add a new subscription with the specified 'nSessionId', that should
  // be updated every 'nPeriosMs' milliseconds. If subscription with
  // the specified 'nSessionId' already exists, it will be updated.

  bool remove(uint32_t nSessionId);
  // Remove subscription with the specified 'nSessionId'.

  bool nextUpdate(uint32_t& nSessionId, uint64_t now);
  // Check if there is any subcription, that should be updated at
  // the specified 'now' time. If there is such subscription,
  // load it's sessionId to the specified 'nSessionId' and return true.
  // Otherwise return false.

  size_t total() const { return m_subscriptions.size(); }
  // Return total number of active subscriptions

private:
  void placeItem(size_t i);

private:
  struct Subscription {
    Subscription() = default;
    Subscription(uint32_t nSessionId,
                 uint64_t nMonitoringPeriodUs,
                 uint64_t nNextUpdate)
      : m_nSessionId(nSessionId)
      , m_nMonitoringPeriodUs(nMonitoringPeriodUs)
      , m_nNextUpdate(nNextUpdate)
    {}

    uint32_t m_nSessionId = 0;
    uint64_t m_nMonitoringPeriodUs = 0;
    uint64_t m_nNextUpdate = 0;
  };

  std::vector<Subscription> m_subscriptions;
};

}  //namespace Utils
