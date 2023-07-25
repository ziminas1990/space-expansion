#include <AdministratorPanel/Spawn.h>

#include <math.h>

#include <SystemManager.h>
#include <World/CelestialBodies/Asteroid.h>
#include <Blueprints/Ships/ShipBlueprint.h>
#include <Modules/BaseModule.h>
#include <Modules/Ship/Ship.h>
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

void SpawnLogic::spawnShip(uint32_t                nSessionId,
                           const std::string&      sPlayerLogin,
                           std::string_view        sBlueprintName,
                           std::string_view        sShipName,
                           const geometry::Point&  position,
                           const geometry::Vector& velocity)
{
  world::PlayerPtr pPlayer =
    m_pSystemManager->getPlayers()->getPlayer(sPlayerLogin);

  if (!pPlayer) {
    sendProblem(nSessionId, admin::Spawn::PLAYER_DOESNT_EXIST);
    return;
  }

  blueprints::BaseBlueprintPtr pBlueprint =
    pPlayer->getBlueprints().getBlueprint(
      blueprints::BlueprintName::make(sBlueprintName));

  if (!pBlueprint) {
    sendProblem(nSessionId, admin::Spawn::BLUEPRINT_DOESNT_EXIST);
    return;
  }

  blueprints::ShipBlueprintPtr pShipBlueprint =
    std::dynamic_pointer_cast<blueprints::ShipBlueprint>(pBlueprint);
  if (!pShipBlueprint) {
    sendProblem(nSessionId, admin::Spawn::NOT_A_SHIP_BLUEPRINT);
    return;
  }

  modules::ShipPtr pNewShip = pShipBlueprint->build(
    std::string(sShipName), pPlayer, pPlayer->getBlueprints());
  if (!pNewShip) {
    sendProblem(nSessionId, admin::Spawn::CANT_SPAWN_SHIP);
    return;
  }
  pNewShip->moveTo(position);
  pNewShip->setVelocity(velocity);
  pPlayer->onNewShip(pNewShip);
  sendShipId(nSessionId, pNewShip->getShipId());
}

void SpawnLogic::handleMessage(uint32_t nSessionId, const admin::Spawn &message)
{
  switch(message.choice_case()) {
    case admin::Spawn::kAsteroid: {
      admin::Message response;
      response.set_timestamp(utils::GlobalClock::now());
      response.mutable_spawn()->set_asteroid_id(
            spawnAsteroid(message.asteroid()));
      m_pChannel->send(nSessionId, std::move(response));
      return;
    }
    case admin::Spawn::kShip: {
      const spex::Position& position = message.ship().position();
      spawnShip(nSessionId,
                message.ship().player(),
                message.ship().blueprint(),
                message.ship().ship_name(),
                geometry::Point(position.x(), position.y()),
                geometry::Vector(position.vx(), position.vy()));
      return;
    }
    default: {
      return;
    }
  }
}

bool SpawnLogic::sendShipId(uint32_t nSessionId, uint32_t nShipId)
{
  admin::Message response;
  response.set_timestamp(utils::GlobalClock::now());
  response.mutable_spawn()->set_ship_id(nShipId);
  return m_pChannel && m_pChannel->send(nSessionId, std::move(response));
}

bool SpawnLogic::sendProblem(uint32_t nSessionId, admin::Spawn::Status problem)
{
  admin::Message response;
  response.set_timestamp(utils::GlobalClock::now());
  response.mutable_spawn()->set_problem(problem);
  return m_pChannel && m_pChannel->send(nSessionId, std::move(response));
}

} // namespace administrator
