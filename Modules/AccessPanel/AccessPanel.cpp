#include "AccessPanel.h"
#include <iostream>
#include <Utils/memstream.h>

#include <Network/UdpSocket.h>
#include <sstream>

namespace modules {

class EchoServer : public network::BufferedTerminal
{
protected:
  void handleMessage(network::MessagePtr pMessage, size_t nLength)
  {
    send(pMessage, nLength);
  }
};

void AccessPanel::handleMessage(network::MessagePtr pMessage, size_t nLength)
{
  // HACK :(
  utils::memstream ss(reinterpret_cast<char*>(const_cast<uint8_t*>(pMessage)), nLength);

  std::string sLogin;
  std::string sPassword;
  std::string sIp;
  uint16_t    nPort;
  ss >> sLogin >> sPassword >> sIp >> nPort;

  if (!checkLogin(sLogin, sPassword)) {
    sendLoginFailed("Invalid login or password");
    return;
  }
  if (!m_pConnectionManager) {
    sendLoginFailed("Internal error");
    return;
  }

  network::UdpEndPoint const& localAddress =
      m_pConnectionManager->createUdpConnection(
        network::UdpEndPoint(
          boost::asio::ip::address_v4::from_string(sIp),
          nPort),
        std::make_shared<EchoServer>());

  if (localAddress == network::UdpEndPoint()) {
    sendLoginFailed("Can't creat UDP socket");
    return;
  }
  sendLoginSuccess(localAddress);
}

bool AccessPanel::checkLogin(std::string const& sLogin, std::string const& sPassword)
{
  return sLogin == "admin" && sPassword == "admin";
}

void AccessPanel::sendLoginSuccess(const network::UdpEndPoint &localAddress)
{
  std::stringstream response;
  response << "OK " << localAddress.port();
  send(reinterpret_cast<uint8_t const*>(response.str().c_str()),
       response.str().size());
}

void AccessPanel::sendLoginFailed(std::string const& reason)
{
  std::stringstream response;
  response << "FAILED " << reason;
  send(reinterpret_cast<uint8_t const*>(response.str().c_str()),
       response.str().size());
}

} // namespace modules
