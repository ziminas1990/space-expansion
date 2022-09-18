#include "AccessPanel.h"
#include <iostream>
#include <Utils/memstream.h>

#include <Network/UdpSocket.h>
#include <Network/ProtobufChannel.h>
#include <Network/BufferedProtobufTerminal.h>
#include <Network/SessionMux.h>

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
      message.choice_case() != spex::Message::kAccessPanel) {
    m_pLoginSocket->closeSession(nSessionId);
    return;
  }

  spex::IAccessPanel const& body = message.accesspanel();
  if (body.choice_case() != spex::IAccessPanel::kLogin) {
    m_pLoginSocket->closeSession(nSessionId);
    return;
  }

  spex::IAccessPanel::LoginRequest const& loginRequest = body.login();

  if (!checkLogin(loginRequest.login(), loginRequest.password())) {
    sendLoginFailed(nSessionId, "Invalid login or password");
    return;
  }
  if (!m_pConnectionManager) {
    sendLoginFailed(nSessionId, "Server initialization error #1");
    return;
  }

  auto pPlayerStorage = m_pPlayersStorage.lock();
  if (!pPlayerStorage) {
    sendLoginFailed(nSessionId, "Server initialization error #2");
    return;
  }

  world::PlayerPtr pPlayer = pPlayerStorage->getPlayer(loginRequest.login());
  if (!pPlayer) {
    sendLoginFailed(nSessionId, "Failed to get or spawn player instance");
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

  if (nConnectionId.has_value()) {
    // Each UDP connection starts with a root session, that is attached to
    // player instance.
    const uint32_t nRootSessionId = pPlayer->getSessionMux()->addConnection(
      *nConnectionId, pPlayer
    );
    sendLoginSuccess(nSessionId, nRootSessionId, pPlayerSocket->getLocalAddr());
  } else {
    sendLoginFailed(nSessionId, "Connections limit reached");
  }
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
                                   uint32_t nRootSessionId,
                                   network::UdpEndPoint const& localAddress)
{
  spex::Message message;
  spex::IAccessPanel::AccessGranted* pGranted =
      message.mutable_accesspanel()->mutable_access_granted();
  pGranted->set_port(localAddress.port());
  pGranted->set_session_id(nRootSessionId);
  return send(nSessionId, std::move(message));
}

bool AccessPanel::sendLoginFailed(uint32_t nSessionId, std::string const& reason)
{
  spex::Message message;
  message.mutable_accesspanel()->set_access_rejected(reason);
  return send(nSessionId, std::move(message));
}

} // namespace modules
