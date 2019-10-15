#pragma once

#include <iostream>
#include "Point.h"
#include "Vector.h"

namespace geometry {

std::ostream& operator<<(std::ostream& out, Point const& point);
std::ostream& operator<<(std::ostream& out, Vector const& vector);

} // namespace geometry
