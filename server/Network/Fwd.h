#pragma once

#include <memory>

#define COMPONENT_FWD_DECLARATION(Component) \
  class Component; \
  using Component##Ptr = std::shared_ptr<Component>; \
  using Component##WeakPtr = std::weak_ptr<Component>; \

namespace network {

COMPONENT_FWD_DECLARATION(BufferedTerminal)
COMPONENT_FWD_DECLARATION(IBufferedTerminalFactory)
COMPONENT_FWD_DECLARATION(IBufferedTerminalFactory)
COMPONENT_FWD_DECLARATION(SessionMux)
COMPONENT_FWD_DECLARATION(SessionMuxManager)
COMPONENT_FWD_DECLARATION(UdpSocket)
COMPONENT_FWD_DECLARATION(UdpDispatcher)

}   // namespace network