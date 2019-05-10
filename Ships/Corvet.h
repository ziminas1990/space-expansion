#pragma once

#include <memory>
#include "Ship.h"

namespace ships
{

class Corvet : public Ship
{
public:
  Corvet() : Ship("Convet") {}
};

using CorvetPtr     = std::shared_ptr<Corvet>;
using CorvetWeakPtr = std::weak_ptr<Corvet>;

} // namespace ships
