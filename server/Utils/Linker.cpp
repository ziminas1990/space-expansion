#include <Utils/Linker.h>

#include <memory>

#include <Network/SessionMux.h>
#include <Modules/Commutator/Commutator.h>
#include <Modules/BaseModule.h>
#include <Autotests/TestUtils/Connector.h>

namespace utils {

template<typename FrameType>
static std::function<void()> mutual_link(
   const network::IChannelPtr<FrameType>& pChannel,
   const network::ITerminalPtr<FrameType>& pTerminal)
{
  pChannel->attachToTerminal(pTerminal);
  pTerminal->attachToChannel(pChannel);

  network::IChannelWeakPtr<FrameType> pChannelWeak = pChannel;
  network::ITerminalWeakPtr<FrameType> pTerminalWeak = pTerminal;
  return [pTerminalWeak, pChannelWeak]() {
    network::IChannelPtr<FrameType>  pChannel  = pChannelWeak.lock();
    network::ITerminalPtr<FrameType> pTerminal = pTerminalWeak.lock();
    if (pTerminal) {
      pTerminal->detachFromChannel();
    }
    if (pChannel) {
      pChannel->detachFromTerminal();
    }
  };
}

template<typename FrameType>
static std::function<void()> link_terminal_to_channel(
   const network::IChannelPtr<FrameType>& pChannel,
   const network::ITerminalPtr<FrameType>& pTerminal)
{
  pTerminal->attachToChannel(pChannel);

  network::ITerminalWeakPtr<FrameType> pTerminalWeak = pTerminal;
  return [pTerminalWeak]() {
    network::ITerminalPtr<FrameType> pTerminal = pTerminalWeak.lock();
    if (pTerminal) {
      pTerminal->detachFromChannel();
    }
  };
}

template<typename FrameType>
static std::function<void()> link_channel_to_terminal(
   const network::IChannelPtr<FrameType>& pChannel,
   const network::ITerminalPtr<FrameType>& pTerminal)
{
  pChannel->attachToTerminal(pTerminal);

  network::IChannelWeakPtr<FrameType> pChannelWeak = pChannel;
  return [pChannelWeak]() {
    network::IChannelPtr<FrameType>  pChannel  = pChannelWeak.lock();
    if (pChannel) {
      pChannel->detachFromTerminal();
    }
  };
}

void Linker::link(network::IBinaryChannelPtr pChannel,
                  network::IBinaryTerminalPtr pTerminal)
{
  m_links.emplace_back(mutual_link(pChannel, pTerminal));
}

void Linker::link(network::IPlayerChannelPtr pChannel,
                  network::IPlayerTerminalPtr pTerminal)
{
  m_links.emplace_back(mutual_link(pChannel, pTerminal));
}

void Linker::link(network::IPrivilegedChannelPtr pChannel,
                  network::IPrivilegedTerminalPtr pTerminal)
{
  m_links.emplace_back(mutual_link(pChannel, pTerminal));
}

void Linker::link(network::SessionMuxPtr pSessionMux,
                  network::IPlayerTerminalPtr pTerminal)
{
  m_links.emplace_back(
    link_terminal_to_channel(pSessionMux->asChannel(), pTerminal));
}

void Linker::link(autotests::PlayerConnectorPtr pConnector,
                  network::IPlayerTerminalPtr pServerSide,
                  autotests::client::IPlayerTerminalPtr  pClientSide)
{
  pConnector->attachToTerminal(pServerSide);
  pConnector->attachToTerminal(pClientSide);
  pServerSide->attachToChannel(pConnector);
  pClientSide->attachToDownlevel(pConnector);

  autotests::PlayerConnectorWeakPtr pConnectorWeak = pConnector;
  m_links.emplace_back([pConnectorWeak]() {
    autotests::PlayerConnectorPtr pConnector = pConnectorWeak.lock();
    if (pConnector) {
      pConnector->detachFromTerminal();
    }
  });
}

uint32_t Linker::attachModule(const modules::CommutatorPtr& pCommutator,
                              const modules::BaseModulePtr& pModule)
{
  const uint32_t nSlotId = pCommutator->attachModule(pModule);

  modules::CommutatorWeakPtr pCommutatorWeak = pCommutator;
  modules::BaseModuleWeakPtr pModuleWeak = pModule;
  m_links.emplace_back(
    [nSlotId, pCommutatorWeak, pModuleWeak]() {
      modules::CommutatorPtr pCommutator = pCommutatorWeak.lock();
      modules::BaseModulePtr pModule     = pModuleWeak.lock();
      if (pCommutator && pModule) {
        pCommutator->detachModule(nSlotId, pModule);
      }
    }
  );
  return nSlotId;
}

void Linker::addCustomUnlinker(std::function<void()>&& unlinker)
{
  m_links.emplace_back(std::move(unlinker));
}

}  // namespace utils