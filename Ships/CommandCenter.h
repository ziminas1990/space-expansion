#pragma once

#include <memory>
#include "Ship.h"

namespace ships {

class CommandCenter : public Ship
{
public:
  CommandCenter() : Ship("CommandCenter", 400000) {}
};

using CommandCenterPtr     = std::shared_ptr<CommandCenter>;
using CommandCenterWeakPtr = std::weak_ptr<CommandCenter>;

} // namespace modules
