#pragma once

#include <memory>
#include <Autotests/TestUtils/ProtobufSyncPipe.h>

namespace autotests
{

class AccessPointClient
{
public:

  void attachToSyncChannel(ProtobufSyncPipePtr pSyncPipe) { m_pSyncPipe = pSyncPipe; }
  void detachFromSyncChannel() { m_pSyncPipe.reset(); }

  bool sendLoginRequest(std::string const& sLogin, std::string const& sPassword,
                        std::string const& sIP, uint16_t nPort);
  bool waitLoginSuccess(uint16_t& nServerPort);
  bool waitLoginFailed();

private:
  ProtobufSyncPipePtr m_pSyncPipe;
};

using AccessPointClientPtr = std::shared_ptr<AccessPointClient>;

} // namespace autotests
