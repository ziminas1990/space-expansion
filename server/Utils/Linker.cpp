#include <Utils/Linker.h>

#include <memory>

namespace utils {

template<typename FrameType>
static std::function<void()> do_link(
   network::IChannelPtr<FrameType>& pChannel,
   network::ITerminalPtr<FrameType>& pTerminal)
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

void Linker::link(network::IBinaryChannelPtr pChannel,
                  network::IBinaryTerminalPtr pTerminal)
{
  m_unlinkers.emplace_back(do_link(pChannel, pTerminal));
}

void Linker::link(network::IPlayerChannelPtr pChannel,
                  network::IPlayerTerminalPtr pTerminal)
{
  m_unlinkers.emplace_back(do_link(pChannel, pTerminal));
}

void Linker::link(network::IPrivilegedChannelPtr pChannel,
                  network::IPrivilegedTerminalPtr pTerminal)
{
  m_unlinkers.emplace_back(do_link(pChannel, pTerminal));
}

}  // namespace utils