#include <Protocol.pb.h>
#include <World/Resources.h>

namespace utils {

inline world::Resource::Type convert(spex::ResourceType eType)
{
  switch (eType) {
    case spex::ResourceType::RESOURCE_ICE:
      return world::Resource::eIce;
    case spex::ResourceType::RESOURCE_METTALS:
      return world::Resource::eMettal;
    case spex::ResourceType::RESOURCE_SILICATES:
      return world::Resource::eSilicate;
    case spex::ResourceType::RESOURCE_LABOR:
      return world::Resource::eLabor;
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
    case world::Resource::eMettal:
      return spex::ResourceType::RESOURCE_METTALS;
    case world::Resource::eSilicate:
      return spex::ResourceType::RESOURCE_SILICATES;
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

}
