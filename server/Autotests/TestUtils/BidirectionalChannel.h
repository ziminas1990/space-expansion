#pragma once

#include <map>
#include <memory>
#include <Network/Interfaces.h>
#include <Autotests/TestUtils/DirectChannel.h>

namespace autotests
{

class BidirectionalChannel
{
private:
  DirectPlayerChannelPtr m_pTx;
    // Is used to send requests from client to server

  DirectPlayerChannelPtr m_pRx;
    // Is used to send responses from server to client

public:
  BidirectionalChannel()
    : m_pTx(std::make_shared<DirectPlayerChannel>())
    , m_pRx(std::make_shared<DirectPlayerChannel>())
  {}

  ~BidirectionalChannel() {
    unlink();
  }

  void link(network::IPlayerTerminalPtr pClientSide,
            network::IPlayerTerminalPtr pServerSide)
    // Set up a linke between 'pClientSide' and 'pServerSide'
  {
    pClientSide->attachToChannel(m_pTx);
    pServerSide->attachToChannel(m_pRx);
    m_pTx->attachToTerminal(pServerSide);
    m_pRx->attachToTerminal(pClientSide);
  }

  void unlink() {
    m_pTx->detachFromTerminal();
    m_pRx->detachFromTerminal();
  }

};

using BidirectionalChannelPtr = std::shared_ptr<BidirectionalChannel>;

} // namespace autotests
