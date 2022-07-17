#include "BaseModule.h"

#include <random>
#include <SystemManager.h>

namespace modules {

void BaseModule::installOn(modules::Ship* pShip)
{
  m_pPlatform = pShip;
  if (m_pPlatform)
    onInstalled(pShip);
}

bool BaseModule::openSession(uint32_t nSessionId)
{
  std::lock_guard<utils::Mutex> guard(m_mutex);
  if (m_activeSessons.size() < gSessionsLimit) {  // [[likely]]
    m_activeSessons.push_back(nSessionId);
    return true;
  }
  return false;
}

void BaseModule::onSessionClosed(uint32_t nSessionId)
{
  std::lock_guard<utils::Mutex> guard(m_mutex);
  // Linear search here :(
  for (size_t i = 0; i < m_activeSessons.size(); ++i) {
    if (m_activeSessons[i] == nSessionId) {
      m_activeSessons[i] = m_activeSessons.back();
      m_activeSessons.pop_back();
    }
  }
}

void BaseModule::handleMessage(uint32_t nSessionId, spex::Message const& message)
{
  // Lock is not required here: message are never handled concurrently for
  // particular module
  if (isOnline()) {
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
      case spex::Message::kPassiveScanner: {
        handlePassiveScannerMessage(nSessionId, message.passive_scanner());
        return;
      }
      case spex::Message::kAccessPanel: {
        // Only AccessPanel is able to handle such massaged, but it is NOT a subclass of
        // BaseModule class
        return;
      }
      case spex::Message::kSession:
      case spex::Message::kGame:
      case spex::Message::CHOICE_NOT_SET: {
        // Just ignoring
        return;
      }
    }
  } else {
    closeSession(nSessionId);
  }
}

} // namespace modules
