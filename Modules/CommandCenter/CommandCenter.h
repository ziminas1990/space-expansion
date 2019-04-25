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
  void handleMessage(size_t nSessionId, spex::CommandCenterMessage&& message) override;
  void onNavigationMessage(size_t nSessionId, spex::INavigation const& message);

private:
  network::IProtobufChannelWeakPtr m_pChannel;
};

using CommandCenterPtr     = std::shared_ptr<CommandCenter>;
using CommandCenterWeakPtr = std::weak_ptr<CommandCenter>;

} // namespace modules
