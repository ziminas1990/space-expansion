#include "SubscriptionsBox.h"

namespace utils {

void SubscriptionsBox::add(uint32_t nSessionId,
                           uint32_t nPeriodMs,
                           uint64_t now)
{
  const uint64_t nPeriodUs = nPeriodMs * 1000;
  const size_t   total     = m_subscriptions.size();
  // Check if subscription already exist
  // O(n), but total number of subscriptions should be small
  for (size_t i = 0; i < total; ++i) {
    if (m_subscriptions[i].m_nSessionId == nSessionId) {
      m_subscriptions[i].m_nMonitoringPeriodUs = nPeriodUs;
      m_subscriptions[i].m_nNextUpdate         = now + nPeriodUs;
      placeItem(i);
      return;
    }
  }

  m_subscriptions.emplace_back(nSessionId, nPeriodUs, now + nPeriodUs);
  placeItem(total);
}

bool SubscriptionsBox::remove(uint32_t nSessionId)
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

bool SubscriptionsBox::nextUpdate(uint32_t& nSessionId, uint64_t now)
{
  if (m_subscriptions.empty()) {
    return false;
  }

  if (m_subscriptions.front().m_nNextUpdate > now) {
    // Most likely path
    return false;
  }

  // Subscription should be updated
  Subscription subscription = m_subscriptions.front();  // NOT a ref!
  nSessionId                = subscription.m_nSessionId;
  do {
    subscription.m_nNextUpdate += subscription.m_nMonitoringPeriodUs;
  } while (subscription.m_nNextUpdate <= now);

  // Keep subscriptions vector sorted
  const uint64_t nNextUpdate = subscription.m_nNextUpdate;
  const size_t   nTotal      = m_subscriptions.size();
  for (size_t i = 1; i < nTotal; ++i) {
    if (nNextUpdate > m_subscriptions[i].m_nNextUpdate) {
      m_subscriptions[i-1] = m_subscriptions[i];
    } else {
      m_subscriptions[i-1] = subscription;
      return true;
    }
  }
  m_subscriptions.back() = subscription;
  return true;
}

void SubscriptionsBox::placeItem(size_t i)
{
  // Move item #i to the correct position in the 'm_subscribers'
  // vector in order to keep vector elements sorted
  Subscription item = m_subscriptions[i];  // NOT a ref!
  const bool lMovingLeft =
      i && m_subscriptions[i-1].m_nNextUpdate > item.m_nNextUpdate;

  if (lMovingLeft) {
    while(i) {
      if (m_subscriptions[i-1].m_nNextUpdate > item.m_nNextUpdate) {
        m_subscriptions[i] = m_subscriptions[i-1];
        --i;
      } else {
        m_subscriptions[i] = item;
        return;
      }
    }
  } else {
    const size_t total = m_subscriptions.size();
    while(i < total - 1) {
      if (item.m_nNextUpdate > m_subscriptions[i+1].m_nNextUpdate) {
        m_subscriptions[i] = m_subscriptions[i+1];
        ++i;
      } else {
        m_subscriptions[i] = item;
        return;
      }
    }
  }
  // i = 0 or total - 1
  m_subscriptions[i] = item;
}

}  // namespace Utils
