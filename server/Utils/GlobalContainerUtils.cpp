#include <Utils/GlobalContainerUtils.h>

#include <iostream>

#include <Utils/GlobalContainer.h>
#include <Modules/Fwd.h>
#include <Network/Fwd.h>
#include <Newton/PhysicalObject.h>
#include <World/CelestialBodies/Asteroid.h>

#define CHECK_CONTAINER_EMPTY(ItemType, problem) \
  if (!GlobalContainer<ItemType>::Empty()) { \
    problem << "GlobalContainer for " #ItemType " contains " << \
    GlobalContainer<ItemType>::Total() << " items"; \
    return false; \
  }

namespace utils {

bool GlobalContainerUtils::checkAllContainersAreEmpty(std::ostream& problem)
{
  CHECK_CONTAINER_EMPTY(modules::AsteroidMiner, problem);
  CHECK_CONTAINER_EMPTY(modules::AsteroidScanner, problem);
  CHECK_CONTAINER_EMPTY(modules::BlueprintsStorage, problem);
  CHECK_CONTAINER_EMPTY(modules::CelestialScanner, problem);
  CHECK_CONTAINER_EMPTY(modules::Commutator, problem);
  CHECK_CONTAINER_EMPTY(modules::Engine, problem);
  CHECK_CONTAINER_EMPTY(modules::PassiveScanner, problem);
  CHECK_CONTAINER_EMPTY(modules::ResourceContainer, problem);
  CHECK_CONTAINER_EMPTY(modules::Ship, problem);
  CHECK_CONTAINER_EMPTY(modules::Shipyard, problem);
  CHECK_CONTAINER_EMPTY(modules::SystemClock, problem);
  CHECK_CONTAINER_EMPTY(newton::PhysicalObject, problem);
  CHECK_CONTAINER_EMPTY(world::Asteroid, problem);
  return true;
}

}  // namespace utils