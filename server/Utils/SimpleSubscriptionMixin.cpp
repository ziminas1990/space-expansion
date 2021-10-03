#include "SimpleSubscriptionMixin.h"

namespace Utils {

void SimpleSubscriptionMixin::addSubscription(uint32_t nSessionId,
                                              uint32_t nPeriodMs,
                                              uint64_t now)
{
  const uint64_t nPeriodUs = nPeriodMs * 1000;
  m_subscriptions.emplace_back(nSessionId, nPeriodUs, now + nPeriodUs);
  // Move it to correct position (keep subscriptions vector sorted)
  for (size_t i = m_subscriptions.size(); i > 0; --i) {
    if (m_subscriptions[i].m_nNextUpdate < m_subscriptions[i-1].m_nNextUpdate) {
      std::swap(m_subscriptions[i], m_subscriptions[i - 1]);
    } else {
      break;
    }
  }
}

bool SimpleSubscriptionMixin::removeSubscription(uint32_t nSessionId)
{
  size_t i = 0;
  const size_t nTotal = m_subscriptions.size();
  for (; i < nTotal; ++i) {
    if (m_subscriptions[i].m_nSessionId == nSessionId) {
      break;
    }
  }
  if (i == nTotal) {
    return false;
  }
  // Move removed element to the end of the vector
  for (; i < nTotal - 1; ++i) {
    std::swap(m_subscriptions[i], m_subscriptions[i + 1]);
  }
  // Pop removed element
  m_subscriptions.pop_back();
  return true;
}

bool SimpleSubscriptionMixin::nextUpdate(uint32_t& nSessionId,
                                         uint64_t now)
{
  if (m_subscriptions.empty()) {
    return false;
  }

  Subscription& subscription = m_subscriptions.front();
  if (subscription.m_nNextUpdate > now) {
    // Most likely path
    return false;
  }

  // Subscription should be updated
  nSessionId = subscription.m_nSessionId;
  do {
    subscription.m_nNextUpdate += subscription.m_nMonitoringPeriodUs;
  } while (subscription.m_nNextUpdate < now);

  // Keep subscriptions vector sorted
  const uint64_t nNextUpdate = subscription.m_nNextUpdate;
  const size_t nTotal = m_subscriptions.size();
  for (size_t i = 1; i < nTotal; ++i) {
    if (nNextUpdate > m_subscriptions[i].m_nNextUpdate) {
      std::swap(m_subscriptions[i-1], m_subscriptions[i]);
    } else {
      break;
    }
  }
  return true;
}

}  // namespace Utils
