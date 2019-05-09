#pragma once

#include <map>
#include <memory>
#include <Network/Interfaces.h>

namespace autotests
{

// Allows two terminals communicate with each other
//
//  +------------+          +----------------------+          +------------+
//  |  Terminal  | <------> | BidirectionalChannel | <------> |  Terminal  |
//  +------------+          +----------------------+          +------------+
//

class BidirectionalChannel : public network::IProtobufChannel
{
public:

  bool createNewSession(uint32_t nSessionId, network::IProtobufTerminalPtr pClient);

  // overrides from IChannel interface
  bool send(uint32_t nSessionId, spex::Message&& message) const override;
  void closeSession(uint32_t nSessionId) override;
  bool isValid() const override;
  void attachToTerminal(network::IProtobufTerminalPtr pServer) override;
  void detachFromTerminal() override { m_pServer.reset(); }

private:
  enum Direction {
    eDirectionUnknown,
    eForward,   // from client to server (request/command)
    eBackward   // from server to client (response/notification)
  };

  Direction determineDirection(spex::Message const& message) const;
  Direction determineDirection(spex::ICommutator const& message) const;
  Direction determineDirection(spex::INavigation const& message) const;

  network::IProtobufTerminalPtr getClientForSession(uint32_t nSessionId) const;

private:
  network::IProtobufTerminalPtr m_pServer;
  // SessionId -> Terminal
  std::map<uint32_t, network::IProtobufTerminalPtr> m_pClients;
};

using BidirectionalChannelPtr = std::shared_ptr<BidirectionalChannel>;

} // namespace autotests
