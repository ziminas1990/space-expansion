#pragma once

#include <vector>
#include <functional>

#include <Network/Interfaces.h>
#include <Autotests/TestUtils/Connector.h>
#include <Network/Fwd.h>
#include <Modules/Fwd.h>

namespace utils {

class Linker {
private:

  // Stores a link. Actually, the only purpose of this class is to
  // destroy a stored link. So, it's an RAII approach to manage links.
  class Link {
  private:
    std::function<void()> m_fUnlinkCallback;

  public:
    Link(std::function<void()>&& unlinker)
    : m_fUnlinkCallback(std::move(unlinker))
    {}
    Link(Link&& other) noexcept = default;

    ~Link() {
      if (m_fUnlinkCallback) {
        m_fUnlinkCallback();
      }
    }

  private:
    Link(const Link& other) = delete;
  };

  std::vector<Link> m_links;

public:

  void link(network::IBinaryChannelPtr pChannel,
            network::IBinaryTerminalPtr pTerminal);

  void link(network::IPlayerChannelPtr pChannel,
            network::IPlayerTerminalPtr pTerminal);

  void link(network::IPrivilegedChannelPtr pChannel,
            network::IPrivilegedTerminalPtr pTerminal);

  void link(network::SessionMuxPtr pSessionMux,
            network::IPlayerTerminalPtr pTerminal);

  void link(autotests::PlayerConnectorPtr pConnector,
            network::IPlayerTerminalPtr pServerSide,
            autotests::client::IPlayerTerminalPtr  pClientSide);



  uint32_t attachModule(const modules::CommutatorPtr& pCommutator,
                        const modules::BaseModulePtr& pModule);

  void addCustomUnlinker(std::function<void()>&& unlinker);

};

} // namespace utils