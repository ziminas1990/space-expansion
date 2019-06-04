#pragma once

#include <Autotests/ClientSDK/SyncPipe.h>

namespace autotests { namespace client {

class ClientAccessPanel
{
public:

  void attachToChannel(SyncPipePtr pSyncPipe) { m_pSyncPipe = pSyncPipe; }
  void detachChannel() { m_pSyncPipe.reset(); }
  SyncPipePtr getSyncChannel() { return m_pSyncPipe; }

  bool login(std::string const& sLogin, std::string const& sPassword,
             std::string const& sLocalIP, uint16_t nLocalPort,
             uint16_t& nRemotePort);

  // Additional functions, that are used in autotests:
  bool sendLoginRequest(std::string const& sLogin, std::string const& sPassword,
                        std::string const& sIP, uint16_t nPort);
  bool waitLoginSuccess(uint16_t& nServerPort);
  bool waitLoginFailed();

private:
  SyncPipePtr m_pSyncPipe;
};

using ClientAccessPanelPtr = std::shared_ptr<ClientAccessPanel>;

}}  // namespace autotests::client
