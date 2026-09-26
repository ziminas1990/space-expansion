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
    m_building.frozen = true;
    if (sendIndication)
      sendBuildingReport(spex::IShipyard::BUILD_FROZEN, m_building.progress);
    return;
  }

  m_building.frozen = false;
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
    case spex::IShipyard::kMonitoring:
      monitoring(nTunnelId);
      return;
    default:
      return;
  }
}

void Shipyard::onSessionClosed(uint32_t nSessionId)
{
  m_monitoringSessions.removeFirst(nSessionId);
  if (m_nBuilderSession == nSessionId) {
    m_nBuilderSession = 0;
  }
  BaseModule::onSessionClosed(nSessionId);
}

void Shipyard::finishBuildingProcedure()
{
  modules::ShipPtr pNewShip = m_building.pShipBlueprint->build(
        m_building.sShipName,
        getOwner(),
        m_building.localLibraryCopy);

  world::PlayerPtr pOwner = getOwner().lock();

  if (!pNewShip || !pOwner) {
    assert(pNewShip != nullptr);
    assert(pOwner != nullptr);
    sendBuildingReport(spex::IShipyard::BUILD_FAILED, m_building.progress);
    m_nBuilderSession = 0;
    switchToIdleState();
    return;
  }

  const modules::Ship* pPlatform = getPlatform();
  pNewShip->moveTo(pPlatform->getPosition());
  pNewShip->setVelocity(pPlatform->getVelocity());
  pNewShip->setOrientation(pPlatform->getOrientation());

  const uint32_t nSlotId = pOwner->onNewShip(pNewShip);
  if (nSlotId == modules::Commutator::invalidSlot()) {
    sendBuildingReport(spex::IShipyard::BUILD_FAILED, m_building.progress);
    m_nBuilderSession = 0;
    switchToIdleState();
    return;
  }

  sendBuildingReport(spex::IShipyard::BUILD_COMPLETE, 1.0);
  sendBuildComplete(std::string(pNewShip->getModuleName()), nSlotId);

  m_nBuilderSession = 0;
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
    sendBuildingReport(nSessionId, spex::IShipyard::SHIPYARD_IS_BUSY, 0);
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
  m_building.sBlueprintName = req.blueprint_name();
  m_building.sShipName = req.ship_name();

  m_nBuilderSession = nSessionId;
  switchToActiveState();

  spex::Message started;
  spex::IShipyard::BuildStarted* pStarted =
      started.mutable_shipyard()->mutable_build_started();
  pStarted->set_blueprint_name(m_building.sBlueprintName);
  pStarted->set_ship_name(m_building.sShipName);
  sendToBuildListeners(started);
}

void Shipyard::cancelBuildReq(uint32_t)
{
  assert("Cancel build is NOT implemented yet");
}

void Shipyard::monitoring(uint32_t nSessionId)
{
  m_monitoringSessions.push(nSessionId);
  sendMonitoringAck(nSessionId);
  if (isIdle()) {
    return;
  }

  sendBuildStarted(nSessionId);
  sendBuildingReport(nSessionId, currentBuildStatus(), m_building.progress);
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

void Shipyard::sendMonitoringAck(uint32_t nSessionId) const
{
  spex::Message message;
  message.mutable_shipyard()->set_monitoring_ack(true);
  sendToClient(nSessionId, std::move(message));
}

void Shipyard::sendBuildStarted(uint32_t nSessionId) const
{
  spex::Message message;
  spex::IShipyard::BuildStarted* pBody =
      message.mutable_shipyard()->mutable_build_started();
  pBody->set_blueprint_name(m_building.sBlueprintName);
  pBody->set_ship_name(m_building.sShipName);
  sendToClient(nSessionId, std::move(message));
}

void Shipyard::sendBuildingReport(spex::IShipyard::Status eStatus, double progress)
{
  spex::Message message;
  spex::IShipyard::BuildingReport* pBody =
      message.mutable_shipyard()->mutable_building_report();
  pBody->set_status(eStatus);
  pBody->set_progress(progress);
  sendToBuildListeners(message);
}

void Shipyard::sendBuildingReport(uint32_t nSessionId,
                                  spex::IShipyard::Status eStatus,
                                  double progress) const
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
  sendToBuildListeners(message);
}

void Shipyard::sendToBuildListeners(spex::Message const& message)
{
  if (m_nBuilderSession != 0) {
    if (!sendToClient(m_nBuilderSession, spex::Message(message))) {
      m_nBuilderSession = 0;
    }
  }

  for (size_t i = 0; i < m_monitoringSessions.size();) {
    const uint32_t nSessionId = m_monitoringSessions[i];
    if (nSessionId == m_nBuilderSession) {
      ++i;
      continue;
    }
    if (!sendToClient(nSessionId, spex::Message(message))) {
      m_monitoringSessions.remove(i);
    } else {
      ++i;
    }
  }
}

spex::IShipyard::Status Shipyard::currentBuildStatus() const
{
  return m_building.frozen
      ? spex::IShipyard::BUILD_FROZEN
      : spex::IShipyard::BUILD_IN_PROGRESS;
}

} // namespace modules
