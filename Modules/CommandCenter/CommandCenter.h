#pragma once

#include <memory>
#include <Newton/PhysicalObject.h>
#include <Network/BufferedProtobufTerminal.h>

namespace modules {

class CommandCenter :
    public network::BufferedProtobufTerminal,
    public newton::PhysicalObject
{
private:
  void handleMessage(spex::CommandCenterMessage const& message);
  void onNavigationMessage(spex::INavigation const& message);

private:
  network::IProtobufChannelWeakPtr m_pChannel;
};

using CommandCenterPtr = std::shared_ptr<CommandCenter>;

} // namespace modules
