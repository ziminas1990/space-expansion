#pragma once

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Autotests/ClientSDK/ClientBaseModule.h>

namespace autotests { namespace client {

class ClientShip : public ClientBaseModule
{
public:
  bool getPosition(geometry::Point& position, geometry::Vector& velocity);
};

}} // namespace autotests
