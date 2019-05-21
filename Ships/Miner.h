#pragma once

#include <memory>
#include "Ship.h"

namespace ships
{

class Miner : public Ship
{
public:
  Miner() : Ship("Miner", 35000) {}
};

using MinerPtr     = std::shared_ptr<Miner>;
using MinerWeakPtr = std::weak_ptr<Miner>;

} // namespace ships
