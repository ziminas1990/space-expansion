#pragma once

#include <Network/Interfaces.h>
#include <World/ObjectTypes.h>
#include <Geometry/Rectangle.h>
#include <World/ObjectContainers.h>

class SystemManager;

namespace tools {
  class ObjectsFilteringManager;
  class RectangeFilter;

  using ObjectsFilteringManagerPtr = std::shared_ptr<ObjectsFilteringManager>;
  using RectangeFilterPtr = std::shared_ptr<RectangeFilter>;
}

namespace newton {
  class PhysicalObject;
}

namespace administrator {

class Screen
{
public:
  Screen();

  void setup(SystemManager* pSystemManager, network::IPrivilegedChannelPtr pChannel);

  void proceed(uint32_t nIntervalUs);
  void handleMessage(uint32_t nSessionId, admin::Screen const& message);

private:
  void move(uint32_t nSessionId, admin::Screen::Position const& position);
  void show(uint32_t nSessionId, admin::ObjectType eType);

  bool sendStatus(uint32_t nSessionId, admin::Screen::Status eStatus);

private:
  network::IPrivilegedChannelPtr    m_pChannel;
    // Channel to client

  tools::ObjectsFilteringManagerPtr m_pFilterManager;
    // The filter manager instance, where the 'm_pFilter' filter will be
    // registered and handled

  world::ContainersCache            m_containersCache;
    // Cache that will be used to get containers, when they are required

  tools::RectangeFilterPtr          m_pFilter;
    // Filter, that will be applied to objects from the 'm_pObjects'
    // container

  uint32_t m_nSessionId = network::gInvalidSessionId;
    // Session, which sent last "show" command
};

} // namespace administrator
