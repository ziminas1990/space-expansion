#pragma once

#include <memory>
#include "Ship.h"

namespace ships
{

class Zond : public Ship
{
public:
  Zond() : Ship("Zond") {}
};

using ZondPtr     = std::shared_ptr<Zond>;
using ZondWeakPtr = std::weak_ptr<Zond>;

} // namespace ships
