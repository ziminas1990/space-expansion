#include "Ship.h"

#include <yaml-cpp/yaml.h>
#include <Utils/YamlReader.h>
#include <Utils/Clock.h>
#include <World/Player.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::Ship);

namespace modules
{

Ship::Ship(
    std::string const& sShipType,
    std::string sName,
    world::PlayerWeakPtr pOwner,
    double weight,
    double radius)
  : BaseModule(std::string("Ship/") + sShipType,
               std::move(sName),
               std::move(pOwner)),
    newton::PhysicalObject(weight, radius)
{
  GlobalObject<Ship>::registerSelf(this);
  m_pCommutator = std::make_shared<modules::Commutator>(
    getOwner().lock()->getSessionMux()
  );
}

Ship::~Ship()
{
  m_pCommutator->detachFromModules();
  for (auto& kv : m_Modules)
    kv.second->onDoestroyed();
}

bool Ship::loadState(YAML::Node const& source)
{
  if (!PhysicalObject::loadState(
        source, PhysicalObject::LoadMask().loadPosition().loadVelocity()))
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

void Ship::proceed(uint32_t)
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
  const uint32_t nSlot = m_pCommutator->attachModule(pModule);
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

  sendToClient(nSessionId, std::move(message));
}

} // namespace modules
