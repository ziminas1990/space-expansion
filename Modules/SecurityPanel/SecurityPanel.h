#pragma once

#include <memory>
#include <Network/Interfaces.h>
#include "ISecurityPanel.h"

namespace modules {

class SecurityPanel : public ISecurityPanel
{
public:
  // from IProtobufTerminal interface
  void onMessageReceived(spex::CommandCenterMessage const& message) override;
};

using SecurityPanelPtr = std::shared_ptr<SecurityPanel>;

}  // namespace modules
