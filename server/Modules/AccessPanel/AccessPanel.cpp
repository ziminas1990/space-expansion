#include "AccessPanel.h"
#include <iostream>
#include <Utils/memstream.h>

#include <Network/UdpSocket.h>
#include <Network/ProtobufChannel.h>
#include <Network/BufferedProtobufTerminal.h>

#include <sstream>

namespace modules {

bool AccessPanel::prephare(uint16_t, uint32_t, uint64_t)
{
  handleBufferedMessages();
  return false;
}

void AccessPanel::handleMessage(uint32_t nSessionId, spex::Message const& message)
{
  std::optional<network::UdpEndPoint> clientAddr = 
      m_pLoginSocket->getRemoteAddr(nSessionId);

  if (!clientAddr.has_value() ||
      message.choice_case() != spex::Message::kAccessPanel)
    return;

  spex::IAccessPanel const& body = message.accesspanel();
  if (body.choice_case() != spex::IAccessPanel::kLogin)
    return;

  spex::IAccessPanel::LoginRequest const& loginRequest = body.login();

  if (!checkLogin(loginRequest.login(), loginRequest.password())) {
    sendLoginFailed(nSessionId, "Invalid login or password");
    return;
  }
  if (!m_pConnectionManager) {
    sendLoginFailed(nSessionId, "Internal error");
    return;
  }

  auto pPlayerStorage = m_pPlayersStorage.lock();
  if (!pPlayerStorage) {
    sendLoginFailed(nSessionId, "Can't create CommandCenter");
    return;
  }

  world::PlayerPtr pPlayer = pPlayerStorage->getPlayer(loginRequest.login());
  if (!pPlayer) {
    sendLoginFailed(nSessionId, "Failed to get or spawn player");
    return;
  }

  network::UdpSocketPtr pPlayerSocket = pPlayer->getUdpSocket();

  if (!pPlayerSocket) {
    pPlayerSocket = m_pConnectionManager->createUdpSocket();
    if (!pPlayerSocket) {
      sendLoginFailed(nSessionId, "Can't create UDP socket");
      return;
    }
    pPlayer->attachToUdpSocket(pPlayerSocket);
  }

  std::optional<uint32_t> nConnectionId = 
      pPlayerSocket->createPersistentSession(*clientAddr);

  if (!nConnectionId) {
    sendLoginFailed(nSessionId, "All slots are busy");
    return;
  }

  sendLoginSuccess(nSessionId, pPlayerSocket->getLocalAddr());
}

bool AccessPanel::checkLogin(std::string const& sLogin,
                             std::string const& sPassword) const
{
  auto pPlayerStorage = m_pPlayersStorage.lock();
  if (!pPlayerStorage)
    return false;
  world::PlayerPtr pPlayer = pPlayerStorage->getPlayer(sLogin);
  if (!pPlayer)
    return false;
  return pPlayer->getLogin()    == sLogin
      && pPlayer->getPassword() == sPassword;
}

bool AccessPanel::sendLoginSuccess(uint32_t nSessionId,
                                   network::UdpEndPoint const& localAddress)
{
  spex::Message message;
  message.mutable_accesspanel()->set_access_granted(localAddress.port());
  return send(nSessionId, message);
}

bool AccessPanel::sendLoginFailed(uint32_t nSessionId, std::string const& reason)
{
  spex::Message message;
  message.mutable_accesspanel()->set_access_rejected(reason);
  return send(nSessionId, message);
}

} // namespace modules
