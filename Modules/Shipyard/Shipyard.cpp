#include "Shipyard.h"
#include <Utils/YamlReader.h>
#include <Ships/Ship.h>
#include <Blueprints/Ships/ShipBlueprint.h>
#include <World/Player.h>
#include <Modules/Commutator/Commutator.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::Shipyard);

namespace modules {



Shipyard::Shipyard(std::string &&sName, world::PlayerWeakPtr pOwner,
                   double laborPerSecond, std::string sContainerName)
  : BaseModule(TypeName(), std::move(sName), std::move(pOwner)),
    m_laborPerSecond(laborPerSecond), m_sContainerName(std::move(sContainerName))
{
  utils::GlobalContainer<Shipyard>::registerSelf(this);
}

bool Shipyard::loadState(YAML::Node const& /*data*/)
{
  return true;
}

void Shipyard::proceed(uint32_t nIntervalUs)
{
  assert(isActive());

  double nIntervalSec  = nIntervalUs / 1000000.0;
  double laborProduced = m_laborPerSecond * nIntervalSec;

  double progressInc = laborProduced / m_building.resources[world::Resource::eLabor];
  if (m_building.progress + progressInc > 1)
    progressInc = 1 - m_building.progress;

  // Calculating total amount of resources, that were consumed during last iteration
  world::ResourcesArray resourcesToConsume;
  for (size_t i = 0; i < world::Resource::eTotalResources; ++i) {
    resourcesToConsume[i] = m_building.resources[i] * progressInc;
  }

  // If all consumed resources are in container, then consume them. Otherwise do not
  // consume anything and send freeze inication
  if (!m_building.pContainer->consumeExactly(resourcesToConsume)) {
    sendBuildStatus(spex::IShipyard::BUILD_FREEZED);
    return;
  }

  m_building.progress += progressInc;

  m_building.nIntervalSinceLastProgressInd += nIntervalUs;
  if (m_building.nIntervalSinceLastProgressInd > 500000) {
    m_building.nIntervalSinceLastProgressInd %= 500000;
    sendBuildProgress(m_building.progress);
  }

  if (m_building.progress > 0.9999) {
    // Ship has been built. Now it should be added to player's commutator
    finishBuildingProcedure();
  }
}

bool Shipyard::openSession(uint32_t nSessionId)
{
  m_openedSessions.insert(nSessionId);
  return true;
}

void Shipyard::onSessionClosed(uint32_t nSessionId)
{
  m_openedSessions.erase(nSessionId);
}

void Shipyard::handleShipyardMessage(uint32_t nTunnelId,
                                     spex::IShipyard const& message)
{
  switch(message.choice_case()) {
    case spex::IShipyard::kStartBuild:
      startBuildReq(nTunnelId, message.start_build());
      return;
    case spex::IShipyard::kCancelBuild:
      cancelBuildReq(nTunnelId);
      return;
    case spex::IShipyard::kSpecificationReq:
      sendSpeification(nTunnelId);
      return;
    default:
      return;
  }
}

void Shipyard::finishBuildingProcedure()
{
  ships::ShipPtr pNewShip = m_building.pShipBlueprint->build(
        m_building.sShipName,
        getOwner(),
        m_building.localLibraryCopy);

  world::PlayerPtr pOwner = getOwner().lock();

  if (!pNewShip && !pOwner) {
    assert(pNewShip != nullptr);
    assert(pOwner != nullptr);
    sendBuildStatus(spex::IShipyard::BUILD_FAILED);
    return;
  }

  pNewShip->moveTo(getPlatform()->getPosition());
  pNewShip->setVelocity(getPlatform()->getVelocity());
  uint32_t nSlotId = pOwner->getCommutator()->attachModule(pNewShip);
  pNewShip->attachToChannel(pOwner->getCommutator());
  sendBuildComplete(std::move(m_building.sShipName), nSlotId);

  switchToIdleState();
}

void Shipyard::startBuildReq(uint32_t nSessionId, spex::IShipyard::StartBuild const& req)
{
  if (!isIdle()) {
    sendBuildStatus(spex::IShipyard::SHIPYARD_IS_BUSY);
    return;
  }

  ships::Ship*     pPlatform = getPlatform();
  world::PlayerPtr pOwner    = getOwner().lock();

  if (!pPlatform || !pOwner) {
    sendBuildStatus(nSessionId, spex::IShipyard::INTERNAL_ERROR);
    return;
  }

  m_building = BuildingTask();
  m_building.pContainer =
      std::dynamic_pointer_cast<modules::ResourceContainer>(
        pPlatform->getModuleByName(m_sContainerName));
  if (!m_building.pContainer) {
    sendBuildStatus(nSessionId, spex::IShipyard::INTERNAL_ERROR);
    return;
  }

  m_building.localLibraryCopy = pOwner->getBlueprints();
  m_building.pShipBlueprint =
      std::dynamic_pointer_cast<blueprints::ShipBlueprint>(
        m_building.localLibraryCopy.getBlueprint(
          blueprints::BlueprintName::make(req.blueprint_name())));
  if (!m_building.pShipBlueprint ||
      !m_building.pShipBlueprint->checkDependencies(m_building.localLibraryCopy)) {
    sendBuildStatus(nSessionId, spex::IShipyard::BLUEPRINT_NOT_FOUND);
    return;
  }

  m_building.pShipBlueprint->exportTotalExpenses(
        m_building.localLibraryCopy, m_building.resources);
  m_building.sShipName = req.ship_name();

  switchToActiveState();
  sendBuildStatus(nSessionId, spex::IShipyard::BUILD_STARTED);
}

void Shipyard::cancelBuildReq(uint32_t nSessionId)
{
  assert("Cancel build is NOT implemented yet");
}

void Shipyard::sendSpeification(uint32_t nSessionId)
{
  spex::Message message;
  spex::IShipyard::Specification* pBody =
      message.mutable_shipyard()->mutable_specification();
  pBody->set_labor_per_sec(m_laborPerSecond);
  sendToClient(nSessionId, message);
}

void Shipyard::sendBuildStatus(spex::IShipyard::Status eStatus)
{
  spex::Message message;
  message.mutable_shipyard()->set_building_status(eStatus);
  for (uint32_t nSessionId : m_openedSessions)
    sendToClient(nSessionId, message);
}

void Shipyard::sendBuildStatus(uint32_t nSessionId, spex::IShipyard::Status eStatus)
{
  spex::Message message;
  message.mutable_shipyard()->set_building_status(eStatus);
  sendToClient(nSessionId, message);
}

void Shipyard::sendBuildProgress(double progress)
{
  spex::Message message;
  message.mutable_shipyard()->set_building_progress(progress);
  for (uint32_t nSessionId : m_openedSessions)
    sendToClient(nSessionId, message);
}

void Shipyard::sendBuildComplete(std::string&& sShipName, uint32_t nSlotId)
{
  spex::Message message;
  spex::IShipyard::ShipBuilt* pBody =
      message.mutable_shipyard()->mutable_building_complete();
  pBody->set_slot_id(nSlotId);
  pBody->set_ship_name(std::move(sShipName));
  for (uint32_t nSessionId : m_openedSessions)
    sendToClient(nSessionId, message);
}

} // namespace modules
