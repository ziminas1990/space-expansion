#include "BidirectionalChannel.h"
#include <iostream>

namespace autotests
{

bool BidirectionalChannel::createNewSession(
    uint32_t nSessionId, network::IProtobufTerminalPtr pClient)
{
  if (!m_pServer || !m_pServer->openSession(nSessionId))
    return false;
  m_pClients[nSessionId] = pClient;
  return true;
}

bool BidirectionalChannel::send(uint32_t nSessionId, spex::Message const& message) const
{
  Direction eDirection = determineDirection(message);
  if (eDirection == eForward) {
    //std::cout << "Client -> Server: " << message.ShortDebugString() << std::endl;
    m_pServer->onMessageReceived(nSessionId, message);
    return true;
  } else {
    //std::cout << "Server -> Client: " << message.ShortDebugString() << std::endl;
    network::IProtobufTerminalPtr pClient = getClientForSession(nSessionId);
    if (pClient) {
      pClient->onMessageReceived(nSessionId, message);
      return true;
    }
  }
  return false;
}

void BidirectionalChannel::closeSession(uint32_t nSessionId)
{
  m_pServer->onSessionClosed(nSessionId);
  network::IProtobufTerminalPtr pClient = getClientForSession(nSessionId);
  if (pClient) {
    pClient->onSessionClosed(nSessionId);
    m_pClients.erase(nSessionId);
  }
}

bool BidirectionalChannel::isValid() const
{
  return !m_pClients.empty() && m_pServer;
}

void BidirectionalChannel::attachToTerminal(network::IProtobufTerminalPtr pServer)
{
  m_pServer = pServer;
}

BidirectionalChannel::Direction
BidirectionalChannel::determineDirection(spex::Message const& message) const
{
  switch(message.choice_case()) {
    case spex::Message::kEncapsulated:
      return determineDirection(message.encapsulated());
    case spex::Message::kCommutator:
      return determineDirection(message.commutator());
    case spex::Message::kNavigation:
      return determineDirection(message.navigation());
    case spex::Message::kAccessPanel:
      return determineDirection(message.accesspanel());
    case spex::Message::kEngine:
      return determineDirection(message.engine());
    case spex::Message::CHOICE_NOT_SET:
      return eDirectionUnknown;
  }
  return eDirectionUnknown;
}

BidirectionalChannel::Direction
BidirectionalChannel::determineDirection(spex::ICommutator const& message) const
{
  switch(message.choice_case()) {
    case spex::ICommutator::kGetTotalSlots:
    case spex::ICommutator::kGetModuleInfo:
    case spex::ICommutator::kGetAllModulesInfo:
    case spex::ICommutator::kOpenTunnel:
    case spex::ICommutator::kCloseTunnel:
      return eForward;
    case spex::ICommutator::kTotalSlotsResponse:
    case spex::ICommutator::kModuleInfo:
    case spex::ICommutator::kOpenTunnelSuccess:
    case spex::ICommutator::kOpenTunnelFailed:
    case spex::ICommutator::kTunnelClosed:
      return eBackward;
    case spex::ICommutator::CHOICE_NOT_SET:
      return eDirectionUnknown;
  }
  return eDirectionUnknown;
}

BidirectionalChannel::Direction
BidirectionalChannel::determineDirection(const spex::INavigation &message) const
{
  switch(message.choice_case()) {
    case spex::INavigation::kPositionRequest:
      return eForward;
    case spex::INavigation::kPositionResponse:
      return eBackward;
    case spex::INavigation::CHOICE_NOT_SET:
      return eDirectionUnknown;
  }
  return eDirectionUnknown;
}

BidirectionalChannel::Direction
BidirectionalChannel::determineDirection(spex::IAccessPanel const& message) const
{
  switch(message.choice_case()) {
    case spex::IAccessPanel::kLogin:
      return eForward;
    case spex::IAccessPanel::kLoginSuccess:
    case spex::IAccessPanel::kLoginFailed:
      return eBackward;
    case spex::IAccessPanel::CHOICE_NOT_SET:
      return eDirectionUnknown;
  }
  return eDirectionUnknown;
}

BidirectionalChannel::Direction
BidirectionalChannel::determineDirection(spex::IEngine const& message) const
{
  switch(message.choice_case()) {
    case spex::IEngine::kGetSpecification:
    case spex::IEngine::kSetThrust:
      return eForward;
    case spex::IEngine::kCurrentThrust:
    case spex::IEngine::kSpecification:
      return eBackward;
    case spex::IEngine::CHOICE_NOT_SET:
      return eDirectionUnknown;
  }
  return eDirectionUnknown;
}

network::IProtobufTerminalPtr
BidirectionalChannel::getClientForSession(uint32_t nSessionId) const
{
  auto I = m_pClients.find(nSessionId);
  return (I == m_pClients.end()) ? nullptr : I->second;
}

} // namespace autotests
