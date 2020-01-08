#include <Protocol.pb.h>
#include <World/Resources.h>

namespace utils {

inline world::Resources::Type convert(spex::ResourceType eType)
{
  switch (eType) {
    case spex::ResourceType::RESOURCE_ICE:
      return world::Resources::eIce;
    case spex::ResourceType::RESOURCE_METTALS:
      return world::Resources::eMettal;
    case spex::ResourceType::RESOURCE_SILICATES:
      return world::Resources::eSilicate;
    case spex::ResourceType::RESOURCE_UNKNOWN:
    case spex::ResourceType::ResourceType_INT_MAX_SENTINEL_DO_NOT_USE_:
    case spex::ResourceType::ResourceType_INT_MIN_SENTINEL_DO_NOT_USE_: {
      // to avoid warning
      assert(nullptr == "Unexpected resource type!");
    }
  }
  return world::Resources::eUnknown;
}

inline spex::ResourceType convert(world::Resources::Type eType)
{
  switch (eType) {
    case world::Resources::eIce:
      return spex::ResourceType::RESOURCE_ICE;
    case world::Resources::eMettal:
      return spex::ResourceType::RESOURCE_METTALS;
    case world::Resources::eSilicate:
      return spex::ResourceType::RESOURCE_SILICATES;
    case world::Resources::eTotalResources:
    case world::Resources::eUnknown: {
      // to avoid warning
      assert(nullptr == "Unexpected resource type!");
    }
  }
  return spex::ResourceType::RESOURCE_UNKNOWN;
}

}
