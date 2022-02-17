#include <Utils/ItemsConverter.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>

namespace utils {

world::ResourcesArray convert(const spex::Resources &resources) {
  world::ResourcesArray result;
  for (const spex::ResourceItem& item: resources.items()) {
    result.at(convert(item.type())) = item.amount();
  }
  return result;
}

void convert(const spex::Position &item,
             geometry::Point *position,
             geometry::Vector *velocity)
{
  position->x = item.x();
  position->y = item.y();
  velocity->setPosition(item.vx(), item.vy());
}



}  // namespace utils
