#pragma once

#include <memory>
#include <Network/Interfaces.h>

namespace modules {

class ISecurityPanel : public network::IProtobufTerminal
{
public:
  virtual ~ISecurityPanel() = default;
};

using ISecurityPanelPtr = std::shared_ptr<ISecurityPanel>;

}  // namespace modules
