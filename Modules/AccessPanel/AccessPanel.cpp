#include "AccessPanel.h"
#include <iostream>
#include <Utils/memstream.h>

#include <Network/UdpSocket.h>
#include <Network/ProtobufChannel.h>
#include <Network/BufferedProtobufTerminal.h>

#include <Ships/CommandCenter.h>

#include <sstream>

namespace modules {

bool AccessPanel::prephareStage(uint16_t)
{
  handleBufferedMessages();
  return false;
}

void AccessPanel::handleMessage(uint32_t nSessionId, spex::Message const& message)
{
  if (message.choice_case() != spex::Message::kAccessPanel)
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

  // Creating protobuf transport layer
  network::ProtobufChannelPtr pProtobufChannel =
      std::make_shared<network::ProtobufChannel>();

  // Creating low-level transport (UDP)
  network::UdpSocketPtr pLocalSocket =
      m_pConnectionManager->createUdpConnection(pProtobufChannel);
  if (!pLocalSocket) {
    sendLoginFailed(nSessionId, "Can't create UDP socket");
    return;
  }
  pLocalSocket->addRemote(
        network::UdpEndPoint(
          boost::asio::ip::address_v4::from_string(loginRequest.ip()),
          uint16_t(loginRequest.port())));

  // Getting (or spawning new) player instance
  modules::CommutatorPtr pEntryPoint = pPlayerStorage->getPlayer(loginRequest.login());
  if (!pEntryPoint)
    pEntryPoint = pPlayerStorage->spawnPlayer(loginRequest.login(), pProtobufChannel);
  if (!pEntryPoint) {
    sendLoginFailed(nSessionId, "Failed to get or spawn player");
    return;
  }

  sendLoginSuccess(nSessionId, pLocalSocket->getNativeSocket().local_endpoint());
}

bool AccessPanel::checkLogin(std::string const& sLogin, std::string const& sPassword)
{
  return sLogin == "admin" && sPassword == "admin";
}

bool AccessPanel::sendLoginSuccess(
    uint32_t nSessionId, const network::UdpEndPoint &localAddress)
{
  spex::Message message;
  spex::IAccessPanel::LoginSuccess* pBody =
      message.mutable_accesspanel()->mutable_loginsuccess();
  pBody->set_port(localAddress.port());
  return send(nSessionId, message);
}

bool AccessPanel::sendLoginFailed(uint32_t nSessionId, std::string const& reason)
{
  spex::Message message;
  spex::IAccessPanel::LoginFailed* pBody =
      message.mutable_accesspanel()->mutable_loginfailed();
  pBody->set_reason(reason);
  return send(nSessionId, message);
}

} // namespace modules
