#include "Ship.h"

#include <cmath>
#include <yaml-cpp/yaml.h>

#include <Modules/Commutator/Commutator.h>
#include <Utils/YamlReader.h>
#include <Utils/Clock.h>
#include <World/Player.h>

namespace {

constexpr double kAngleEpsilon = 1e-8;

} // namespace

DECLARE_GLOBAL_CONTAINER_CPP(modules::Ship);

namespace modules
{

Ship::Ship(
    std::string sBlueprintName,
    std::string sName,
    world::PlayerWeakPtr pOwner,
    double weight,
    double radius,
    double maxRotationSpeed)
  : BaseModule(TypeName(),
               std::move(sName),
               std::move(pOwner),
               std::move(sBlueprintName)),
    newton::PhysicalObject(weight, radius),
    m_maxRotationSpeed(maxRotationSpeed > 0.0 ? maxRotationSpeed : 0.0)
{
  GlobalObject<Ship>::registerSelf(this);
  m_pCommutator = std::make_shared<modules::Commutator>(
    getOwner().lock()->getSessionMux()
  );
}

bool Ship::loadState(YAML::Node const& source)
{
  if (!PhysicalObject::loadState(
        source,
        PhysicalObject::LoadMask()
            .loadPosition()
            .loadVelocity()
            .loadOrientation()))
    return false;

  // Loading state of modules
  for (auto const& kv : source["modules"]) {
    std::string const& sModuleName = kv.first.as<std::string>();
    auto I = m_Modules.find(sModuleName);
    if (I == m_Modules.end())
      return false;
    modules::BaseModulePtr& pModule = I->second;
    if (!pModule->loadState(kv.second)) {
      return false;
    }
  }
  return true;
}

void Ship::proceed(uint32_t /*nIntervalUs*/)
{
  const uint64_t now = utils::GlobalClock::now();

  // Send updates to subscribers
  uint32_t session;
  while (m_subscriptions.nextUpdate(session, now)) {
    sendState(session);
  }
}

uint32_t Ship::installModule(modules::BaseModulePtr pModule)
{
  if (m_Modules.find(pModule->getModuleName()) != m_Modules.end()) {
    return modules::Commutator::invalidSlot();
  }
  m_Modules.insert(std::make_pair(pModule->getModuleName(), pModule));
  const uint32_t nSlot = m_linker.attachModule(m_pCommutator, pModule);
  pModule->installOn(this);
  return nSlot;
}

void Ship::onMessageReceived(uint32_t nSessionId, spex::Message const& message)
{
  if (message.choice_case() == spex::Message::kCommutator) {
    // Forwarding message to commutator
    m_pCommutator->onMessageReceived(nSessionId, message);
  } else {
    BaseModule::onMessageReceived(nSessionId, message);
  }
}

void Ship::attachToChannel(network::IPlayerChannelPtr pChannel)
{
  BaseModule::attachToChannel(pChannel);
  m_pCommutator->attachToChannel(pChannel);
}

void Ship::detachFromChannel()
{
  BaseModule::detachFromChannel();
  m_pCommutator->detachFromChannel();
}

modules::BaseModulePtr Ship::getModuleByName(std::string const& sName) const
{
  auto I = m_Modules.find(sName);
  return I != m_Modules.end() ? I->second : modules::BaseModulePtr();
}

void Ship::onSessionClosed(uint32_t nSessionId)
{
  m_subscriptions.remove(nSessionId);
  BaseModule::onSessionClosed(nSessionId);
}

void Ship::handleShipMessage(uint32_t nSessionId, spex::IShip const& message)
{
  switch (message.choice_case()) {
    case spex::IShip::kStateReq: {
      sendState(nSessionId);
      return;
    }
    case spex::IShip::kMonitor: {
      handleMonitorRequest(nSessionId, message.monitor());
      return;
    }
    case spex::IShip::kSpecificationReq: {
      sendSpecification(nSessionId);
      return;
    }
    case spex::IShip::kRotate: {
      handleRotate(nSessionId, message.rotate());
      return;
    }
    default: {
      return;
    }
  }
}

void Ship::handleNavigationMessage(uint32_t nSessionId,
                                   spex::INavigation const& message)
{
  switch (message.choice_case()) {
    case spex::INavigation::kPositionReq: {
      spex::Message response;
      spex::Position* pBody = response.mutable_navigation()->mutable_position();
      pBody->set_x(getPosition().x);
      pBody->set_y(getPosition().y);
      pBody->set_vx(getVelocity().getX());
      pBody->set_vy(getVelocity().getY());
      sendToClient(nSessionId, std::move(response));
      return;
    }
    default: {
      return;
    }
  }
}

void Ship::handleMonitorRequest(uint32_t nSessionId, uint32_t nPeriodMs)
{
  if (nPeriodMs && nPeriodMs < 100) {
    nPeriodMs = 100;
  } else if (nPeriodMs > 60000) {
    nPeriodMs = 60000;
  }
  sendState(nSessionId);
  if (nPeriodMs) {
    m_subscriptions.add(nSessionId, nPeriodMs, utils::GlobalClock::now());
    switchToActiveState();
  } else {
    m_subscriptions.remove(nSessionId);
  }
}

void Ship::sendState(uint32_t nSessionId, int eStateMask) const
{
  spex::Message message;
  spex::IShip::State* pBody = message.mutable_ship()->mutable_state();

  if (eStateMask & StateMask::eWeight) {
    pBody->mutable_weight()->set_value(getWeight());
  }

  if (eStateMask & StateMask::ePosition) {
    spex::Position* pPosition = pBody->mutable_position();
    pPosition->set_x(getPosition().x);
    pPosition->set_y(getPosition().y);
    pPosition->set_vx(getVelocity().getX());
    pPosition->set_vy(getVelocity().getY());
  }

  if (eStateMask & StateMask::eOrientation) {
    spex::IShip::Direction* pOrientation = pBody->mutable_orientation();
    pOrientation->set_x(getOrientation().getX());
    pOrientation->set_y(getOrientation().getY());
  }

  sendToClient(nSessionId, std::move(message));
}

void Ship::sendSpecification(uint32_t nSessionId) const
{
  spex::Message message;
  spex::IShip::Specification* pSpecification =
      message.mutable_ship()->mutable_specification();
  pSpecification->set_max_rotation_speed(m_maxRotationSpeed);
  pSpecification->set_radius(getRadius());
  sendToClient(nSessionId, std::move(message));
}

void Ship::handleRotate(uint32_t nSessionId, spex::IShip::Rotate const& request)
{
  spex::Message ack;
  ack.mutable_ship()->set_rotate_ack(true);
  sendToClient(nSessionId, std::move(ack));

  geometry::Vector target(request.x(), request.y());
  if (!(target.getLength() > 0.0)) {
    return;
  }
  target.normalize();

  const double speed = std::min(request.speed(), m_maxRotationSpeed);
  if (!(speed > 0.0)) {
    rotate(0.0, 0);
    return;
  }

  const double angle = getOrientation().shortestTurn(target);
  if (std::abs(angle) <= kAngleEpsilon) {
    setOrientation(target);
    rotate(0.0, 0);
    return;
  }

  const double signedSpeed = angle > 0.0 ? speed : -speed;
  const double durationUs = std::abs(angle) / speed * 1000000.0;
  rotate(signedSpeed, static_cast<uint64_t>(std::llround(durationUs)));
}

} // namespace modules
