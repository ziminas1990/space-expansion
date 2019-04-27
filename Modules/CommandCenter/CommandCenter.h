#pragma once

#include <memory>
#include <Newton/PhysicalObject.h>
#include <Modules/BaseModule.h>

namespace modules {

class CommandCenter : public BaseModule, public newton::PhysicalObject
{
public:
  CommandCenter() : BaseModule("Ship/CommandCenter") {}

protected:
  void handleNavigationMessage(
      size_t nSessionId, spex::INavigation const& message) override;

private:
  network::IProtobufChannelWeakPtr m_pChannel;
};

using CommandCenterPtr     = std::shared_ptr<CommandCenter>;
using CommandCenterWeakPtr = std::weak_ptr<CommandCenter>;

} // namespace modules
