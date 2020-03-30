#pragma once

#include <map>
#include <memory>
#include <Network/Interfaces.h>
#include <Autotests/ClientSDK/Interfaces.h>

namespace autotests
{

// Allows two terminals communicate with each other
//
//  +------------+          +----------------------+          +------------+
//  |  Terminal  | <------> | BidirectionalChannel | <------> |  Terminal  |
//  +------------+          +----------------------+          +------------+
//

class BidirectionalChannel :
    public client::IPlayerChannel,
    public network::IPlayerChannel
{
public:

  void attachToClientSide(client::IPlayerTerminalWeakPtr pClientLink);

  // overrides from IChannel interface
  bool send(uint32_t nSessionId, spex::Message const& message) const override;
  void closeSession(uint32_t) override {}
  bool isValid() const override;
  void attachToTerminal(network::IPlayerTerminalPtr pServer) override;
  void detachFromTerminal() override { m_pServer.reset(); }

  // overrides from client::IClientChannel
  bool send(spex::Message const& message) override;

private:
  network::IPlayerTerminalPtr  m_pServer;
  client::IPlayerTerminalWeakPtr m_pClientLink;
};

using BidirectionalChannelPtr = std::shared_ptr<BidirectionalChannel>;

} // namespace autotests
