#include "Ship.h"

#include <yaml-cpp/yaml.h>
#include <Utils/YamlReader.h>

#include <SystemManager.h>

DECLARE_GLOBAL_CONTAINER_CPP(ships::Ship);

namespace ships
{

Ship::Ship(std::string const& sShipType, std::string sName, world::PlayerWeakPtr pOwner,
           double weight, double radius)
  : BaseModule(std::string("Ship/") + sShipType, std::move(sName), std::move(pOwner)),
    newton::PhysicalObject(weight, radius),
    m_pCommutator(std::make_shared<modules::Commutator>())
{
  GlobalContainer<Ship>::registerSelf(this);
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
  if (m_monitors.empty()) {
    switchToIdleState();
  }

  for (auto& [nSessionId, monitor]: m_monitors) {
    uint64_t dtUs = SystemManager::getIngameTime() - monitor.m_lastUpdateUs;
    if (dtUs < monitor.m_periodUs) {
      continue;
    }
    monitor.m_lastUpdateUs += (dtUs - dtUs % monitor.m_periodUs);
    sendState(nSessionId);
  }
}

bool Ship::installModule(modules::BaseModulePtr pModule)
{
  if (m_Modules.find(pModule->getModuleName()) != m_Modules.end())
    return false;
  m_Modules.insert(std::make_pair(pModule->getModuleName(), pModule));
  m_pCommutator->attachModule(pModule);
  pModule->attachToChannel(m_pCommutator);
  pModule->installOn(this);
  return true;
}

void Ship::onMessageReceived(uint32_t nSessionId, spex::Message const& message)
{
  if (message.choice_case() == spex::Message::kCommutator ||
      message.choice_case() == spex::Message::kEncapsulated) {
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
  std::map<std::string, modules::BaseModulePtr>::const_iterator I = m_Modules.find(sName);
  return I != m_Modules.end() ? I->second : modules::BaseModulePtr();
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

void Ship::handleNavigationMessage(uint32_t nSessionId, spex::INavigation const& message)
{
  switch (message.choice_case()) {
    case spex::INavigation::kPositionReq: {
      spex::Message response;
      spex::Position* pBody = response.mutable_navigation()->mutable_position();
      pBody->set_x(getPosition().x);
      pBody->set_y(getPosition().y);
      pBody->set_vx(getVelocity().getX());
      pBody->set_vy(getVelocity().getY());
      sendToClient(nSessionId, response);
      return;
    }
    default: {
      return;
    }
  }
}

void Ship::handleMonitorRequest(uint32_t nSessionId, uint32_t nPeriodMs)
{
  if (m_monitors.size() > 8) {
    sendMonitorAck(nSessionId, 0);
    return;
  }

  if (nPeriodMs && nPeriodMs < 100) {
    nPeriodMs = 100;
  } else if (nPeriodMs > 60000) {
    nPeriodMs = 60000;
  }
  sendMonitorAck(nSessionId, nPeriodMs);

  if (nPeriodMs) {
    MonitoringSession& session = m_monitors[nSessionId];
    session.m_periodUs         = nPeriodMs * 1000;
    session.m_lastUpdateUs     = SystemManager::getIngameTime();
    sendState(nSessionId);
    switchToActiveState();
  } else {
    m_monitors.erase(nSessionId);
    if (m_monitors.empty()) {
      switchToIdleState();
    }
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

  sendToClient(nSessionId, message);
}

void Ship::sendMonitorAck(uint32_t nSessionId, uint32_t nPeriodMs) const
{
  spex::Message message;
  message.mutable_ship()->set_monitor_ack(nPeriodMs);
  sendToClient(nSessionId, message);
}

} // namespace modules
