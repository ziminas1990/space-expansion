#pragma once

#include <memory>
#include <string>

#include <Network/BufferedProtobufTerminal.h>
#include <Utils/YamlForwardDeclarations.h>
#include <Utils/RandomSequence.h>
#include <Utils/Mutex.h>

namespace world {
class Player;
using PlayerWeakPtr = std::weak_ptr<world::Player>;
}

namespace modules
{

class Ship;

// Each module should inherite BaseModule class
class BaseModule : public network::BufferedPlayerTerminal
{
private:
  enum class Status {
    eOffline,
    eOnline,
    eDestoyed
  };

  enum class State {
    eIdle,
    eActivating,
    eActive,
    eDeactivating
  };

public:
  BaseModule(std::string sModuleType,
             std::string moduleName,
             world::PlayerWeakPtr pOwner);

  virtual bool loadState(YAML::Node const& /*source*/) { return true; }
  virtual void proceed(uint32_t /*nIntervalUs*/) { switchToIdleState(); }

  std::string const& getModuleType() const { return m_sModuleType; }
  std::string const& getModuleName() const { return m_sModuleName; }

  void putOffline()         { m_eStatus = Status::eOffline; }
  void putOnline()          { m_eStatus = Status::eOnline; }
  void onDoestroyed()       { m_eStatus = Status::eDestoyed; }
  bool isOffline()    const { return m_eStatus == Status::eOffline; }
  bool isOnline()     const { return m_eStatus == Status::eOnline; }
  bool isDestroyed()  const { return m_eStatus == Status::eDestoyed; }

  void onDeactivated() {
    assert(m_eState == State::eDeactivating);
    m_eState = State::eIdle;
  }
  void onActivated() {
    assert(m_eState == State::eActivating);
    m_eState = State::eActive;
  }
  bool isIdle()         const { return m_eState == State::eIdle; }
  bool isActivating()   const { return m_eState == State::eActivating; }
  bool isActive()       const { return m_eState == State::eActive; }
  bool isDeactivating() const { return m_eState == State::eDeactivating; }

  void installOn(modules::Ship* pShip);

  world::PlayerWeakPtr getOwner() const { return m_pOwner; }

  // from IProtobufTerminal:
  // By default, there is no reason to reject new session opening and there is
  // nothing to do, when some session has been closed
  bool canOpenSession() const override;
  void openSession(uint32_t nSessionId) override;
  void onSessionClosed(uint32_t nSessionId) override;

  void closeActiveSessions();
  bool hasOpenedSessions() const { return !m_activeSessons.empty(); }
  const std::vector<uint32_t> getOpenedSession() const {
    return m_activeSessons;
  }

protected:
  // overrides from BufferedProtobufTerminal interface
  void handleMessage(uint32_t nSessionId, spex::Message const& message) override;

  virtual void handleCommutatorMessage(uint32_t, spex::ICommutator const&) {}
  virtual void handleShipMessage(uint32_t, spex::IShip const&) {}
  virtual void handleNavigationMessage(uint32_t, spex::INavigation const&) {}
  virtual void handleEngineMessage(uint32_t, spex::IEngine const&) {}
  virtual void handleCelestialScannerMessage(uint32_t, spex::ICelestialScanner const&) {}
  virtual void handlePassiveScannerMessage(uint32_t, spex::IPassiveScanner const&) {}
  virtual void handleAsteroidScannerMessage(uint32_t, spex::IAsteroidScanner const&) {}
  virtual void handleResourceContainerMessage(uint32_t, spex::IResourceContainer const&) {}
  virtual void handleAsteroidMinerMessage(uint32_t, spex::IAsteroidMiner const&) {}
  virtual void handleBlueprintsStorageMessage(uint32_t, spex::IBlueprintsLibrary const&) {}
  virtual void handleShipyardMessage(uint32_t, spex::IShipyard const&) {}
  virtual void handleSystemClockMessage(uint32_t, spex::ISystemClock const&) {}
  virtual void handleMessangerMessage(uint32_t, spex::IMessanger const&) {}

  // Will be called once, when module is installed on some ship
  virtual void onInstalled(modules::Ship* /*pPlatform*/) {}

  // returns a pointer to the ship, on which module is installed
  modules::Ship*       getPlatform()       { return m_pPlatform; }
  modules::Ship const* getPlatform() const { return m_pPlatform; }

  inline bool sendToClient(uint32_t nSessionId, spex::Message&& message) const {
    return network::BufferedPlayerTerminal::send(nSessionId, std::move(message));
  }

  void switchToIdleState() {
    if (m_eState != State::eIdle)
      m_eState = State::eDeactivating;
  }
  void switchToActiveState() {
    if (m_eState != State::eActive)
      m_eState = State::eActivating;
  }

private:
  struct Subscription {
    uint32_t m_nSessionId;
    uint32_t m_nToken;
  };

private:
  mutable utils::Mutex  m_mutex;
  std::string           m_sModuleType;
  std::string           m_sModuleName;
  world::PlayerWeakPtr  m_pOwner;
  Status                m_eStatus;
  State                 m_eState;
  modules::Ship*        m_pPlatform = nullptr;
  std::vector<uint32_t> m_activeSessons;
};

using BaseModulePtr      = std::shared_ptr<BaseModule>;
using BaseModuleConstPtr = std::shared_ptr<const BaseModule>;
using BaseModuleWeakPtr  = std::weak_ptr<BaseModule>;

} // namespace modules
