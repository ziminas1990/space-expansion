#include "Ship.h"

#include <yaml-cpp/yaml.h>
#include <Utils/YamlReader.h>

DECLARE_GLOBAL_CONTAINER_CPP(ships::Ship);

namespace ships
{

Ship::Ship(std::string const& sShipType, std::string sName, double weight, double radius)
  : BaseModule(std::string("Ship/") + sShipType, std::move(sName)),
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

  utils::YamlReader reader(source);
  std::string sName;
  reader.read("name", sName);
  changeModuleName(std::move(sName));

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

void Ship::attachToChannel(network::IProtobufChannelPtr pChannel)
{
  BaseModule::attachToChannel(pChannel);
  m_pCommutator->attachToChannel(pChannel);
}

void Ship::detachFromChannel()
{
  BaseModule::detachFromChannel();
  m_pCommutator->detachFromChannel();
}

void Ship::handleShipMessage(uint32_t nSessionId, spex::IShip const& message)
{
  switch (message.choice_case()) {
    case spex::IShip::kStateRequest: {
      spex::IShip response;
      spex::IShip_GetStateResponse* pBody = response.mutable_stateresponse();
      pBody->set_weight(getWeight());
      sendToClient(nSessionId, response);
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
    case spex::INavigation::kPositionRequest: {
      spex::INavigation navigation;
      spex::INavigation_GetPositionResponse* pBody =
          navigation.mutable_positionresponse();
      pBody->set_x(getPosition().x);
      pBody->set_y(getPosition().y);
      pBody->set_vx(getVelocity().getX());
      pBody->set_vy(getVelocity().getY());
      sendToClient(nSessionId, navigation);
      return;
    }
    default: {
      return;
    }
  }
}

} // namespace modules
