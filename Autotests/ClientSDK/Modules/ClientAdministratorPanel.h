#pragma once

#include <memory>

#include <Autotests/ClientSDK/SyncPipe.h>

namespace autotests { namespace client {

class AdministratorPanel
{
public:
  enum Status {
    eSuccess,
    eLoginFailed,

    // Errors, detected on client side:
    eTransportError,
    eUnexpectedMessage,
    eTimeoutError,
    eStatusError
  };

  void attachToChannel(PrivilegedPipePtr pSyncPipe) { m_pPrivilegedPipe = pSyncPipe; }
  void detachChannel() { m_pPrivilegedPipe.reset(); }

  Status login(std::string const& sLogin, std::string const& sPassword,
               uint64_t& nToken);

private:
  PrivilegedPipePtr m_pPrivilegedPipe;
};

using AdministratorPanelPtr = std::shared_ptr<AdministratorPanel>;

}}  // namespace autotests::client
