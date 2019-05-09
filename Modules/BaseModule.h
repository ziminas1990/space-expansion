#pragma once

#include <memory>
#include <string>
#include <Network/BufferedProtobufTerminal.h>

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

  std::string const& getModuleType() const { return m_sModuleType; }

  Status getStatus() const { return m_eStatus; }

  // from IProtobufTerminal:
  // By default, there is no reason to reject new session opening and there is
  // nothing to do, when some session has been closed
  bool openSession(uint32_t /*nSessionId*/) override { return true; }
  void onSessionClosed(uint32_t /*nSessionId*/) override {}

protected:
  void offline()    { m_eStatus = eOffline; }
  void online()     { m_eStatus = eOnline; }
  void doestroyed() { m_eStatus = eDestoyed; }

  // overrides from BufferedProtobufTerminal interface
  void handleMessage(uint32_t nSessionId, spex::Message const& message) override;

  virtual void handleCommutatorMessage(size_t, spex::ICommutator const&) {}
  virtual void handleNavigationMessage(size_t, spex::INavigation const&) {}

  inline bool sendToClient(uint32_t nSessionId, spex::Message const& message) const {
    return network::BufferedProtobufTerminal::send(nSessionId, message);
  }
  bool sendToClient(uint32_t nSessionId, spex::ICommutator const& message) const;
  bool sendToClient(uint32_t nSessionId, spex::INavigation const& message) const;

private:
  std::string m_sModuleType;
  Status      m_eStatus;
};

using BaseModulePtr     = std::shared_ptr<BaseModule>;
using BaseModuleWeakPtr = std::weak_ptr<BaseModule>;

} // namespace modules
