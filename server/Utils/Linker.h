#pragma once

#include <vector>
#include <functional>
#include <Network/Interfaces.h>
#include <Network/Fwd.h>

namespace utils {

class Linker {
private:

  class Unlinker {
  private:
    std::function<void()> m_fUnlinkCallback;

  public:
    Unlinker(std::function<void()>&& unlinker) 
    : m_fUnlinkCallback(std::move(unlinker))
    {}
    Unlinker(Unlinker&& other) noexcept = default;

    ~Unlinker() {
      if (m_fUnlinkCallback) {
        m_fUnlinkCallback();
      }
    }

  private:
    Unlinker(const Unlinker& other) = delete;
  };

  std::vector<Unlinker> m_unlinkers;

public:

  void link(network::IBinaryChannelPtr pChannel,
            network::IBinaryTerminalPtr pTerminal);

  void link(network::IPlayerChannelPtr pChannel,
            network::IPlayerTerminalPtr pTerminal);

  void link(network::IPrivilegedChannelPtr pChannel,
            network::IPrivilegedTerminalPtr pTerminal);

  void link(network::SessionMuxPtr pSessionMux,
            network::IPlayerTerminalPtr pTerminal);

  void link(network::IPlayerChannelPtr pChannel,
            network::SessionMuxPtr pSessionMux);

};

} // namespace utils