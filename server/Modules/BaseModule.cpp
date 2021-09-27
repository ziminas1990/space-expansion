#include "BaseModule.h"

#include <random>
#include <SystemManager.h>

namespace modules {

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

} // namespace modules
