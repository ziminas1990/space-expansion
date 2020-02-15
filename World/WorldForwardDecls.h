#pragma once

#include <memory>

namespace world {

class Player;
using PlayerPtr     = std::shared_ptr<Player>;
using PlayerWeakPtr = std::weak_ptr<Player>;

}  // namespace world
