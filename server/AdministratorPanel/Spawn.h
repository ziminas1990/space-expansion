#pragma once

#include <stdint.h>
#include <Privileged.pb.h>
#include <Network/Interfaces.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>

class SystemManager;

namespace administrator {

class SpawnLogic {
  SystemManager*                 m_pSystemManager;
  network::IPrivilegedChannelPtr m_pChannel;
    // Channel to client

  uint32_t spawnAsteroid(const admin::Spawn::Asteroid& asteroid);
  
  void spawnShip(uint32_t                nSessionId,
                 const std::string&      sPlayerLogin,
                 std::string_view        sBlueprintName,
                 std::string_view        sShipName,
                 const geometry::Point&  position,
                 const geometry::Vector& velocity);

public:

  void setup(SystemManager* pSystemManager,
             network::IPrivilegedChannelPtr pChannel)
  {
    m_pSystemManager = pSystemManager;
    m_pChannel       = pChannel;
  }

  void handleMessage(uint32_t nSessionId, const admin::Spawn& message);

private:
  bool sendShipId(uint32_t nSessionId, uint32_t nShipId);
  bool sendProblem(uint32_t nSessionId, admin::Spawn::Status problem);
};

} // namespace administrator
