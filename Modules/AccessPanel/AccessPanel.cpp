#include "AccessPanel.h"
#include <iostream>
#include <Utils/memstream.h>

#include <Network/UdpSocket.h>
#include <Network/ProtobufChannel.h>
#include <Network/BufferedProtobufTerminal.h>

#include <Modules/CommandCenter/CommandCenter.h>

#include <sstream>

namespace modules {

void AccessPanel::handleMessage(uint32_t nSessionId,
                                network::BinaryMessage const& message)
{
  // HACK :(
  utils::memstream ss(reinterpret_cast<char*>(const_cast<uint8_t*>(message.m_pBody)),
                      message.m_nLength);

  std::string sLogin;
  std::string sPassword;
  std::string sIp;
  uint16_t    nPort;
  ss >> sLogin >> sPassword >> sIp >> nPort;

  if (!checkLogin(sLogin, sPassword)) {
    sendLoginFailed(nSessionId, "Invalid login or password");
    return;
  }
  if (!m_pConnectionManager) {
    sendLoginFailed(nSessionId, "Internal error");
    return;
  }

  network::ProtobufChannelPtr pProtobufChannel =
      std::make_shared<network::ProtobufChannel>();

  network::UdpSocketPtr pLocalSocket =
      m_pConnectionManager->createUdpConnection(pProtobufChannel);

  if (!pLocalSocket) {
    sendLoginFailed(nSessionId, "Can't create UDP socket");
    return;
  }

  pLocalSocket->addRemote(
        network::UdpEndPoint(
          boost::asio::ip::address_v4::from_string(sIp),
          nPort));

  auto pPlayerStorage = m_pPlayersStorage.lock();
  if (!pPlayerStorage) {
    sendLoginFailed(nSessionId, "Can't create CommandCenter");
    return;
  }

  modules::CommandCenterPtr pCommandCenter =
      pPlayerStorage->getOrCreateCommandCenter(std::move(sLogin));
  if (!pCommandCenter) {
    sendLoginFailed(nSessionId, "Failed to create CommandCenter instance");
    return;
  }

  pProtobufChannel->attachToTerminal(pCommandCenter);
  pCommandCenter->attachToChannel(pProtobufChannel);

  sendLoginSuccess(nSessionId, pLocalSocket->getNativeSocket().local_endpoint());
}

bool AccessPanel::checkLogin(std::string const& sLogin, std::string const& sPassword)
{
  return sLogin == "admin" && sPassword == "admin";
}

void AccessPanel::sendLoginSuccess(
    uint32_t nSessionId, const network::UdpEndPoint &localAddress)
{
  std::stringstream response;
  response << "OK " << localAddress.port();
  getChannel()->send(
        nSessionId,
        network::BinaryMessage(response.str().c_str(), response.str().size()));
  getChannel()->closeSession(nSessionId);
}

void AccessPanel::sendLoginFailed(uint32_t nSessionId, std::string const& reason)
{
  std::stringstream response;
  response << "FAILED " << reason;
  getChannel()->send(
        nSessionId,
        network::BinaryMessage(response.str().c_str(), response.str().size()));
  getChannel()->closeSession(nSessionId);
}

} // namespace modules
