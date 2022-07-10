#include <Utils/Linker.h>
#include <Network/SessionMux.h>

#include <memory>

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
  m_unlinkers.emplace_back(mutual_link(pChannel, pTerminal));
}

void Linker::link(network::IPlayerChannelPtr pChannel,
                  network::IPlayerTerminalPtr pTerminal)
{
  m_unlinkers.emplace_back(mutual_link(pChannel, pTerminal));
}

void Linker::link(network::IPrivilegedChannelPtr pChannel,
                  network::IPrivilegedTerminalPtr pTerminal)
{
  m_unlinkers.emplace_back(mutual_link(pChannel, pTerminal));
}

void Linker::link(network::SessionMuxPtr pSessionMux,
                  network::IPlayerTerminalPtr pTerminal)
{
  m_unlinkers.emplace_back(
    link_terminal_to_channel(pSessionMux->asChannel(), pTerminal));
}

void Linker::link(network::IPlayerChannelPtr pChannel,
                  network::SessionMuxPtr pSessionMux)
{
  m_unlinkers.emplace_back(
    link_channel_to_terminal(pChannel, pSessionMux->asTerminal()));
}

}  // namespace utils