#include "AccessPanel.h"
#include <iostream>
#include <Utils/memstream.h>

#include <Network/UdpSocket.h>
#include <Network/ProtobufChannel.h>
#include <Network/BufferedProtobufTerminal.h>

#include <Modules/CommandCenter/CommandCenter.h>

#include <sstream>

namespace modules {

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

  network::ProtobufChannelPtr pProtobufChannel =
      std::make_shared<network::ProtobufChannel>();

  network::UdpEndPoint const& localAddress =
      m_pConnectionManager->createUdpConnection(
        network::UdpEndPoint(
          boost::asio::ip::address_v4::from_string(sIp),
          nPort),
        pProtobufChannel);

  if (localAddress == network::UdpEndPoint()) {
    sendLoginFailed("Can't create UDP socket");
    return;
  }

  auto pPlayerStorage = m_pPlayersStorage.lock();
  if (!pPlayerStorage) {
    sendLoginFailed("Can't create CommandCenter");
    return;
  }

  modules::CommandCenterPtr pCommandCenter =
      pPlayerStorage->getOrCreateCommandCenter(std::move(sLogin));
  if (!pCommandCenter) {
    sendLoginFailed("Failed to create CommandCenter instance");
    return;
  }

  pProtobufChannel->attachToTerminal(pCommandCenter);
  pCommandCenter->attachToChannel(pProtobufChannel);

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

//========================================================================================
// AccessPanelFacotry
//========================================================================================

void AccessPanelFacotry::setCreationData(network::ConnectionManagerPtr pManager,
                                         world::PlayerStorageWeakPtr pPlayersStorage)
{
  m_pManager        = pManager;
  m_pPlayersStorage = pPlayersStorage;
}

network::BufferedTerminalPtr AccessPanelFacotry::make()
{
  AccessPanelPtr pPanel = std::make_shared<AccessPanel>();
  pPanel->attachToConnectionManager(m_pManager);
  pPanel->attachToPlayerStorage(m_pPlayersStorage);
  return std::move(pPanel);
}

} // namespace modules
