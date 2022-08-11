#include <Protocol.pb.h>
#include <World/Resources.h>
#include <Newton/PhysicalObject.h>

namespace geometry {
class Point;
class Vector;
}  // namespace geometry

namespace utils {

inline world::Resource::Type convert(spex::ResourceType eType)
{
  switch (eType) {
    case spex::ResourceType::RESOURCE_ICE:
      return world::Resource::eIce;
    case spex::ResourceType::RESOURCE_METALS:
      return world::Resource::eMetal;
    case spex::ResourceType::RESOURCE_SILICATES:
      return world::Resource::eSilicate;
    case spex::ResourceType::RESOURCE_LABOR:
      return world::Resource::eLabor;
    case spex::ResourceType::RESOURCE_STONE:
      return world::Resource::eStone;
    case spex::ResourceType::RESOURCE_UNKNOWN:
    case spex::ResourceType::ResourceType_INT_MAX_SENTINEL_DO_NOT_USE_:
    case spex::ResourceType::ResourceType_INT_MIN_SENTINEL_DO_NOT_USE_: {
      // to avoid warning
      assert(nullptr == "Unexpected resource type!");
    }
  }
  return world::Resource::eUnknown;
}

inline spex::ResourceType convert(world::Resource::Type eType)
{
  switch (eType) {
    case world::Resource::eIce:
      return spex::ResourceType::RESOURCE_ICE;
    case world::Resource::eMetal:
      return spex::ResourceType::RESOURCE_METALS;
    case world::Resource::eSilicate:
      return spex::ResourceType::RESOURCE_SILICATES;
    case world::Resource::eStone:
      return spex::ResourceType::RESOURCE_STONE;
    case world::Resource::eLabor:
      return spex::ResourceType::RESOURCE_LABOR;
    case world::Resource::eTotalResources:
    case world::Resource::eUnknown: {
      // to avoid warning
      assert(nullptr == "Unexpected resource type!");
    }
  }
  return spex::ResourceType::RESOURCE_UNKNOWN;
}

inline spex::ObjectType convert(world::ObjectType eType)
{
  switch (eType) {
    case world::ObjectType::eShip:
      return spex::ObjectType::OBJECT_SHIP;
    case world::ObjectType::eAsteroid:
      return spex::ObjectType::OBJECT_ASTEROID;
    default:
      assert("Unexpected type" == nullptr);
  }
  return spex::ObjectType::OBJECT_UNKNOWN;
}

inline world::ObjectType convert(spex::ObjectType eType)
{
  switch (eType) {
    case spex::ObjectType::OBJECT_SHIP:
      return world::ObjectType::eShip;
    case spex::ObjectType::OBJECT_ASTEROID:
      return world::ObjectType::eAsteroid;
    default:
      assert("Unexpected type" == nullptr);
  }
  return world::ObjectType::eUnknown;
}

inline void convert(world::ResourceItem const& item, spex::ResourceItem* pOutput)
{
  pOutput->set_type(convert(item.m_eType));
  pOutput->set_amount(item.m_nAmount);
}

inline world::ResourceItem convert(spex::ResourceItem const& item)
{
  world::ResourceItem output;
  output.m_eType   = convert(item.type());
  output.m_nAmount = item.amount();
  return output;
}

world::ResourcesArray convert(const spex::Resources& resources);

void convert(const spex::Position& item,
             geometry::Point* position,
             geometry::Vector* velocity);

inline void convert(const newton::PhysicalObject* pFrom,
                    spex::PhysicalObject* pTo)
{
  pTo->set_id(pFrom->getInstanceId());
  pTo->set_object_type(convert(pFrom->getType()));
  pTo->set_x(pFrom->getPosition().x);
  pTo->set_y(pFrom->getPosition().y);
  pTo->set_vx(static_cast<float>(pFrom->getVelocity().getX()));
  pTo->set_vy(static_cast<float>(pFrom->getVelocity().getY()));
  pTo->set_r(static_cast<float>(pFrom->getRadius()));
}

} // namespace utils
