#pragma once

#include <memory>
#include <string>
#include <Network/BufferedProtobufTerminal.h>

#include <Utils/YamlForwardDeclarations.h>

namespace ships {
class Ship;
}

namespace modules
{

// Each module should inherite BaseModule class
class BaseModule : public network::BufferedProtobufTerminal
{
public:
  enum Status {
    eOffline,
    eOnline,
    eDestoyed
  };

public:
  BaseModule(std::string&& sModuleType)
    : m_sModuleType(std::move(sModuleType)), m_eStatus(eOnline)
  {}

  virtual bool loadState(YAML::Node const& /*source*/) { return true; }

  std::string const& getModuleType() const { return m_sModuleType; }

  void   putOffline()         { m_eStatus = eOffline; }
  void   putOnline()          { m_eStatus = eOnline; }
  void   onDoestroyed()       { m_eStatus = eDestoyed; }
  bool   isOffline()    const { return m_eStatus == eOffline; }
  bool   isOnline()     const { return m_eStatus == eOnline; }
  bool   isDestroyed()  const { return m_eStatus == eDestoyed; }

  void installOn(ships::Ship* pShip);

  // from IProtobufTerminal:
  // By default, there is no reason to reject new session opening and there is
  // nothing to do, when some session has been closed
  bool openSession(uint32_t /*nSessionId*/) override { return true; }
  void onSessionClosed(uint32_t /*nSessionId*/) override {}

protected:
  // overrides from BufferedProtobufTerminal interface
  void handleMessage(uint32_t nSessionId, spex::Message const& message) override;

  virtual void handleCommutatorMessage(uint32_t, spex::ICommutator const&) {}
  virtual void handleNavigationMessage(uint32_t, spex::INavigation const&) {}
  virtual void handleEngineMessage(uint32_t, spex::IEngine const&) {}
  virtual void handleCelestialScannerMessage(uint32_t, spex::ICelestialScanner const&) {}

  // Will be called once, when module is installed on some ship
  virtual void onInstalled(ships::Ship* /*pPlatform*/) {}

  // returns a pointer to the ship, on which module has been installed
  ships::Ship*       getPlatform()       { return m_pPlatform; }
  ships::Ship const* getPlatform() const { return m_pPlatform; }

  inline bool sendToClient(uint32_t nSessionId, spex::Message const& message) const {
    return network::BufferedProtobufTerminal::send(nSessionId, message);
  }
  bool sendToClient(uint32_t nSessionId, spex::ICommutator const& message) const;
  bool sendToClient(uint32_t nSessionId, spex::INavigation const& message) const;
  bool sendToClient(uint32_t nSessionId, spex::IEngine const& message) const;

private:
  std::string  m_sModuleType;
  Status       m_eStatus;
  ships::Ship* m_pPlatform = nullptr;
};

using BaseModulePtr     = std::shared_ptr<BaseModule>;
using BaseModuleWeakPtr = std::weak_ptr<BaseModule>;

} // namespace modules
