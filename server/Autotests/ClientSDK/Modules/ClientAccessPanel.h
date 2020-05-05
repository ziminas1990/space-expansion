#pragma once

#include <Autotests/ClientSDK/ClientBaseModule.h>

namespace autotests { namespace client {

class ClientAccessPanel : public ClientBaseModule
{
public:
  bool login(std::string const& sLogin, std::string const& sPassword,
             std::string const& sLocalIP, uint16_t nLocalPort,
             uint16_t& nRemotePort);

  // Additional functions, that are used in autotests:
  bool sendLoginRequest(std::string const& sLogin, std::string const& sPassword,
                        std::string const& sIP, uint16_t nPort);
  bool waitLoginSuccess(uint16_t& nServerPort);
  bool waitLoginFailed();
};

using ClientAccessPanelPtr = std::shared_ptr<ClientAccessPanel>;

}}  // namespace autotests::client
