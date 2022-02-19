#include <AdministratorPanel/Spawn.h>

#include <math.h>

#include <SystemManager.h>
#include <World/CelestialBodies/Asteroid.h>
#include <Utils/ItemsConverter.h>
#include <Utils/FloatComparator.h>
#include <Utils/Clock.h>
#include <Privileged.pb.h>

namespace administrator {

uint32_t SpawnLogic::spawnAsteroid(const admin::Spawn::Asteroid &asteroid)
{
  world::ResourcesArray composition =
      utils::convert(asteroid.composition()).normalized();
  geometry::Point position;
  geometry::Vector velocity;
  utils::convert(asteroid.position(), &position, &velocity);
  return m_pSystemManager->getWorld().spawnAsteroid(
        composition, asteroid.radius(), position, velocity);
}

void SpawnLogic::handleMessage(uint32_t nSessionId, const admin::Spawn &message)
{
  admin::Message response;
  response.set_timestamp(utils::GlobalClock::now());

  switch(message.choice_case()) {
    case admin::Spawn::kAsteroid: {
      response.mutable_spawn()->set_asteroid_id(
            spawnAsteroid(message.asteroid()));
      m_pChannel->send(nSessionId, response);
      return;
    }
    default: {
      return;
    }
  }
}

} // namespace administrator
