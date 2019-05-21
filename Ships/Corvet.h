#pragma once

#include <memory>
#include "Ship.h"

namespace ships
{

class Corvet : public Ship
{
public:
  Corvet() : Ship("Corvet", 70000) {}
};

using CorvetPtr     = std::shared_ptr<Corvet>;
using CorvetWeakPtr = std::weak_ptr<Corvet>;

} // namespace ships
