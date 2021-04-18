#include "BaseModule.h"

#include <random>
#include <SystemManager.h>

namespace modules {

utils::RandomSequence BaseModule::m_tokenGenerator =
    utils::RandomSequence(static_cast<unsigned int>(std::time(nullptr)));

void BaseModule::installOn(ships::Ship* pShip)
{
  m_pPlatform = pShip;
  if (m_pPlatform)
    onInstalled(pShip);
}

void BaseModule::handleMessage(uint32_t nSessionId, spex::Message const& message)
{
  switch(message.choice_case()) {
    case spex::Message::kCommutator: {
      handleCommutatorMessage(nSessionId, message.commutator());
      return;
    }
    case spex::Message::kNavigation: {
      handleNavigationMessage(nSessionId, message.navigation());
      return;
    }
    case spex::Message::kEngine: {
      handleEngineMessage(nSessionId, message.engine());
      return;
    }
    case spex::Message::kShip: {
      handleShipMessage(nSessionId, message.ship());
      return;
    }
    case spex::Message::kSystemClock: {
      handleSystemClockMessage(nSessionId, message.system_clock());
      return;
    }
    case spex::Message::kCelestialScanner: {
      handleCelestialScannerMessage(nSessionId, message.celestial_scanner());
      return;
    }
    case spex::Message::kMonitor: {
      handleMonitorMessage(nSessionId, message.monitor());
      return;
    }
    case spex::Message::kAsteroidMiner: {
      handleAsteroidMinerMessage(nSessionId, message.asteroid_miner());
      return;
    }
    case spex::Message::kAsteroidScanner: {
      handleAsteroidScannerMessage(nSessionId, message.asteroid_scanner());
      return;
    }
    case spex::Message::kResourceContainer: {
      handleResourceContainerMessage(nSessionId, message.resource_container());
      return;
    }
    case spex::Message::kBlueprintsLibrary: {
      handleBlueprintsStorageMessage(nSessionId, message.blueprints_library());
      return;
    }
    case spex::Message::kShipyard: {
      handleShipyardMessage(nSessionId, message.shipyard());
      return;
    }
    case spex::Message::kAccessPanel: {
      // Only AccessPanel is able to handle such massaged, but it is NOT a subclass of
      // BaseModule class
      return;
    }
    case spex::Message::kEncapsulated: {
      // Only commutator is able to handle such messages!
      return;
    }
    case spex::Message::kGame:
    case spex::Message::CHOICE_NOT_SET: {
      // Just ignoring
      return;
    }
  }
}

void BaseModule::sendUpdate(spex::Message const& message) const
{
  for (Subscription const& subscription: m_subscribers) {
    sendToClient(subscription.m_nSessionId, message);
  }
}

void BaseModule::handleMonitorMessage(uint32_t nSessionId, spex::IMonitor const& monitor)
{
  switch (monitor.choice_case()) {
    case spex::IMonitor::kSubscribe: {
      spex::Message response;
      spex::IMonitor* pBody = response.mutable_monitor();
      if (m_subscribers.size() > 15) {
        pBody->set_status(spex::IMonitor::LIMIT_EXCEEDED);
        sendToClient(nSessionId, response);
        return;
      }
      uint32_t token = generateToken();
      m_subscribers.push_back(Subscription{nSessionId, token});
      pBody->set_token(token);
      sendToClient(nSessionId, response);
      return;
    }
    case spex::IMonitor::kUnsubscribe: {
      spex::Message response;
      spex::IMonitor* pBody = response.mutable_monitor();

      uint32_t token = monitor.unsubscribe();
      size_t i = 0;
      while (i < m_subscribers.size() && m_subscribers[i].m_nToken != token) {
        ++i;
      }
      if (i == m_subscribers.size()) {
        pBody->set_status(spex::IMonitor::INVALID_TOKEN);
        sendToClient(nSessionId, response);
        return;
      }
      m_subscribers[i] = m_subscribers.back();
      m_subscribers.pop_back();
      pBody->set_status(spex::IMonitor::SUCCESS);
      sendToClient(nSessionId, response);
      return;
    }
    case spex::IMonitor::kToken:
    case spex::IMonitor::kStatus:
    case spex::IMonitor::CHOICE_NOT_SET:
      return;
  }
}

uint32_t BaseModule::generateToken() const
{
  uint32_t token  = 0;
  bool     unique = false;
  while (!unique) {
    token  = m_tokenGenerator.yield();
    unique = true;
    for (Subscription const& item: m_subscribers) {
      if (item.m_nToken == token) {
        unique = false;
        break;
      }
    }
  }
  return token;
}

} // namespace modules
