#pragma once

#include <memory>
#include <string>
#include <Network/BufferedProtobufTerminal.h>

#include <Utils/YamlForwardDeclarations.h>

namespace ships {
class Ship;
}

namespace world {
class Player;
using PlayerWeakPtr = std::weak_ptr<world::Player>;
}

namespace modules
{

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
  BaseModule(std::string sModuleType, std::string moduleName, world::PlayerWeakPtr pOwner)
    : m_sModuleType(std::move(sModuleType)),
      m_sModuleName(std::move(moduleName)),
      m_pOwner(std::move(pOwner)),
      m_eStatus(Status::eOnline),
      m_eState(State::eIdle)
  {}

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

  void onDeactivated()        { m_eState = State::eIdle; }
  void onActivated()          { m_eState = State::eActive; }
  bool isIdle()         const { return m_eState == State::eIdle; }
  bool isActivating()   const { return m_eState == State::eActivating; }
  bool isActive()       const { return m_eState == State::eActive; }
  bool isDeactivating() const { return m_eState == State::eDeactivating; }

  void installOn(ships::Ship* pShip);

  world::PlayerWeakPtr getOwner() const { return m_pOwner; }

  // from IProtobufTerminal:
  // By default, there is no reason to reject new session opening and there is
  // nothing to do, when some session has been closed
  bool openSession(uint32_t /*nSessionId*/) override { return true; }
  void onSessionClosed(uint32_t /*nSessionId*/) override {}

protected:
  // overrides from BufferedProtobufTerminal interface
  void handleMessage(uint32_t nSessionId, spex::Message const& message) override;

  virtual void handleCommutatorMessage(uint32_t, spex::ICommutator const&) {}
  virtual void handleShipMessage(uint32_t, spex::IShip const&) {}
  virtual void handleNavigationMessage(uint32_t, spex::INavigation const&) {}
  virtual void handleEngineMessage(uint32_t, spex::IEngine const&) {}
  virtual void handleCelestialScannerMessage(uint32_t, spex::ICelestialScanner const&) {}
  virtual void handleAsteroidScannerMessage(uint32_t, spex::IAsteroidScanner const&) {}
  virtual void handleResourceContainerMessage(uint32_t, spex::IResourceContainer const&) {}
  virtual void handleAsteroidMinerMessage(uint32_t, spex::IAsteroidMiner const&) {}
  virtual void handleBlueprintsStorageMessage(uint32_t, spex::IBlueprintsLibrary const&) {}
  virtual void handleShipyardMessage(uint32_t, spex::IShipyard const&) {}
  virtual void handleSystemClockMessage(uint32_t, spex::ISystemClock const&) {}

  // Will be called once, when module is installed on some ship
  virtual void onInstalled(ships::Ship* /*pPlatform*/) {}

  // returns a pointer to the ship, on which module is installed
  ships::Ship*       getPlatform()       { return m_pPlatform; }
  ships::Ship const* getPlatform() const { return m_pPlatform; }

  inline bool sendToClient(uint32_t nSessionId, spex::Message const& message) const {
    return network::BufferedPlayerTerminal::send(nSessionId, message);
  }

  bool sendToClient(uint32_t nSessionId, spex::ICommutator const& message) const;
  bool sendToClient(uint32_t nSessionId, spex::IShip const& message) const;
  bool sendToClient(uint32_t nSessionId, spex::INavigation const& message) const;
  bool sendToClient(uint32_t nSessionId, spex::IEngine const& message) const;

  void switchToIdleState() {
    if (m_eState == State::eIdle)
      return;
    assert(m_eState == State::eActive || m_eState == State::eDeactivating);
    m_eState = State::eDeactivating;
  }
  void switchToActiveState() {
    if (m_eState == State::eActive)
      return;
    assert(m_eState == State::eIdle || m_eState == State::eActivating);
    m_eState = State::eActivating;
  }

private:
  std::string          m_sModuleType;
  std::string          m_sModuleName;
  world::PlayerWeakPtr m_pOwner;
  Status               m_eStatus;
  State                m_eState;
  ships::Ship*         m_pPlatform = nullptr;
};

using BaseModulePtr     = std::shared_ptr<BaseModule>;
using BaseModuleWeakPtr = std::weak_ptr<BaseModule>;

} // namespace modules
