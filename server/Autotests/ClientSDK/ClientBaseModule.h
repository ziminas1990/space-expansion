#pragma once

#include <Autotests/ClientSDK/SyncPipe.h>

namespace autotests { namespace client {

class ClientBaseModule
{
public:
  enum MonitoringStatus {
    eSuccess,
    eLimitExceeded,
    eInvalidToken,
    eUnexpectedMessage,

    // SDK side errors
    eTimeout,
    eTransportError
  };

public:
  void attachToChannel(PlayerPipePtr pSyncPipe) { m_pSyncPipe = pSyncPipe; }
  void detachChannel() { m_pSyncPipe.reset(); }
  PlayerPipePtr getChannel() { return m_pSyncPipe; }
  bool isAttached() const { return m_pSyncPipe != nullptr; }

  MonitoringStatus subscribe(uint32_t& token);
  MonitoringStatus unsubscribe(uint32_t token);

  // Forwarding interface from SyncPipe
  bool send(spex::Message const& message)
  { return m_pSyncPipe && m_pSyncPipe->send(message); }

  template<typename MessageType>
  bool wait(MessageType& message, uint16_t nTimeout = 500)
  { return m_pSyncPipe && m_pSyncPipe->wait(message, nTimeout); }

  void dropQueuedMessage()
  {
    if (m_pSyncPipe) {
      m_pSyncPipe->dropAll();
    }
  }

private:
  PlayerPipePtr m_pSyncPipe;
};

}} // namespace autotests
