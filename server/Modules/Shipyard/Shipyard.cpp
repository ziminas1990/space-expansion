#include "Shipyard.h"
#include <Utils/YamlReader.h>
#include <Modules/Ship/Ship.h>
#include <Blueprints/Ships/ShipBlueprint.h>
#include <World/Player.h>
#include <Modules/Commutator/Commutator.h>
#include <Modules/ResourceContainer/ResourceContainer.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::Shipyard);

namespace modules {


Shipyard::Shipyard(std::string &&sName,
                   world::PlayerWeakPtr pOwner,
                   double laborPerSecond)
  : BaseModule(TypeName(), std::move(sName), std::move(pOwner)),
    m_laborPerSecond(laborPerSecond)
{
  utils::GlobalObject<Shipyard>::registerSelf(this);
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

  m_building.nIntervalSinceLastInd += nIntervalUs;
  const bool sendIndication = m_building.nIntervalSinceLastInd > 500000;
  if (sendIndication) {
    m_building.nIntervalSinceLastInd %= 500000;
  }

  // If all consumed resources are in container, then consume them. Otherwise do not
  // consume anything and send freeze inication
  if (!m_pContainer || !m_pContainer->consumeExactly(resourcesToConsume)) {
    if (sendIndication)
      sendBuildingReport(spex::IShipyard::BUILD_FROZEN, m_building.progress);
    return;
  }

  m_building.progress += progressInc;

  if (sendIndication) {
    if (m_building.progress > 0.9999) {
      // Ship has been built. Now it should be added to player's commutator
      finishBuildingProcedure();
    } else {
      sendBuildingReport(spex::IShipyard::BUILD_IN_PROGRESS, m_building.progress);
    }
  }
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
    case spex::IShipyard::kBindToCargo:
      bindToCargo(nTunnelId, message.bind_to_cargo());
      return;
    default:
      return;
  }
}

void Shipyard::finishBuildingProcedure()
{
  modules::ShipPtr pNewShip = m_building.pShipBlueprint->build(
        m_building.sShipName,
        getOwner(),
        m_building.localLibraryCopy);

  world::PlayerPtr pOwner = getOwner().lock();

  if (!pNewShip && !pOwner) {
    assert(pNewShip != nullptr);
    assert(pOwner != nullptr);
    sendBuildingReport(spex::IShipyard::BUILD_FAILED, m_building.progress);
    return;
  }

  pNewShip->moveTo(getPlatform()->getPosition());
  pNewShip->setVelocity(getPlatform()->getVelocity());

  const uint32_t nSlotId = pOwner->onNewShip(pNewShip);
  sendBuildingReport(spex::IShipyard::BUILD_COMPLETE, 1.0);
  sendBuildComplete(std::move(m_building.sShipName), nSlotId);

  switchToIdleState();
}

void Shipyard::bindToCargo(uint32_t nSessionId, std::string const& name)
{
  if (name.empty()) {
    m_pContainer = nullptr;
    sendStatus(nSessionId, spex::IShipyard::SUCCESS);
    return;
  }

  modules::ResourceContainerPtr pCargo =
      std::dynamic_pointer_cast<modules::ResourceContainer>(
        getPlatform()->getModuleByName(name));
  if (!pCargo) {
    sendStatus(nSessionId, spex::IShipyard::CARGO_NOT_FOUND);
    return;
  }

  m_pContainer = pCargo;
  sendStatus(nSessionId, spex::IShipyard::SUCCESS);
}

void Shipyard::startBuildReq(uint32_t nSessionId, spex::IShipyard::StartBuild const& req)
{
  if (!isIdle()) {
    sendBuildingReport(spex::IShipyard::SHIPYARD_IS_BUSY, 0);
    return;
  }

  modules::Ship*   pPlatform = getPlatform();
  world::PlayerPtr pOwner    = getOwner().lock();

  if (!pPlatform || !pOwner) {
    sendBuildingReport(nSessionId, spex::IShipyard::INTERNAL_ERROR, 0);
    return;
  }

  if (!m_pContainer) {
    sendBuildingReport(nSessionId, spex::IShipyard::CARGO_NOT_FOUND, 0);
    return;
  }

  m_building = BuildingTask();
  m_building.localLibraryCopy = pOwner->getBlueprints();
  m_building.pShipBlueprint =
      std::dynamic_pointer_cast<blueprints::ShipBlueprint>(
        m_building.localLibraryCopy.getBlueprint(
          blueprints::BlueprintName::make(req.blueprint_name())));
  if (!m_building.pShipBlueprint ||
      !m_building.pShipBlueprint->checkDependencies(m_building.localLibraryCopy)) {
    sendBuildingReport(nSessionId, spex::IShipyard::BLUEPRINT_NOT_FOUND, 0);
    return;
  }

  m_building.pShipBlueprint->exportTotalExpenses(
        m_building.localLibraryCopy, m_building.resources);
  m_building.sShipName = req.ship_name();

  switchToActiveState();
  sendBuildingReport(nSessionId, spex::IShipyard::BUILD_STARTED, 0);
}

void Shipyard::cancelBuildReq(uint32_t)
{
  assert("Cancel build is NOT implemented yet");
}

void Shipyard::sendStatus(uint32_t nSessionId, spex::IShipyard::Status eStatus) const
{
  spex::Message message;
  message.mutable_shipyard()->set_bind_to_cargo_status(eStatus);
  sendToClient(nSessionId, std::move(message));
}

void Shipyard::sendSpeification(uint32_t nSessionId)
{
  spex::Message message;
  spex::IShipyard::Specification* pBody =
      message.mutable_shipyard()->mutable_specification();
  pBody->set_labor_per_sec(m_laborPerSecond);
  sendToClient(nSessionId, std::move(message));
}

void Shipyard::sendBuildingReport(spex::IShipyard::Status eStatus, double progress)
{
  spex::Message message;
  spex::IShipyard::BuildingReport* pBody =
      message.mutable_shipyard()->mutable_building_report();
  pBody->set_status(eStatus);
  pBody->set_progress(progress);
  for (uint32_t nSessionId : getOpenedSession()) {
    sendToClient(nSessionId, std::move(message));
  }
}

void Shipyard::sendBuildingReport(uint32_t nSessionId,
                                  spex::IShipyard::Status eStatus,
                                  double progress)
{
  spex::Message message;
  spex::IShipyard::BuildingReport* pBody =
      message.mutable_shipyard()->mutable_building_report();
  pBody->set_status(eStatus);
  pBody->set_progress(progress);
  sendToClient(nSessionId, std::move(message));
}

void Shipyard::sendBuildComplete(std::string&& sShipName, uint32_t nSlotId)
{
  spex::Message message;
  spex::IShipyard::ShipBuilt* pBody =
      message.mutable_shipyard()->mutable_building_complete();
  pBody->set_slot_id(nSlotId);
  pBody->set_ship_name(std::move(sShipName));
  for (uint32_t nSessionId : getOpenedSession())
    sendToClient(nSessionId, std::move(message));
}

} // namespace modules
