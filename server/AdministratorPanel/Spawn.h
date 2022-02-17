#pragma once

#include <stdint.h>
#include <Privileged.pb.h>
#include <Network/Interfaces.h>

class SystemManager;

namespace administrator {

class SpawnLogic {
  SystemManager*                 m_pSystemManager;
  network::IPrivilegedChannelPtr m_pChannel;
    // Channel to client

  uint32_t spawnAsteroid(const admin::Spawn::Asteroid& asteroid);

public:

  void setup(SystemManager* pSystemManager,
             network::IPrivilegedChannelPtr pChannel)
  {
    m_pSystemManager = pSystemManager;
    m_pChannel       = pChannel;
  }

  void handleMessage(uint32_t nSessionId, const admin::Spawn& message);

};

} // namespace administrator
