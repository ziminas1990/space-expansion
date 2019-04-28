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
  BaseModule(std::string&& sModuleType)
    : m_sModuleType(std::move(sModuleType))
  {}

  std::string const& getModuleType() const { return m_sModuleType; }

  // from IProtobufTerminal:
  // By default, there is no reason to reject new session opening and there is
  // nothing to do, when some session has been closed
  bool openSession(uint32_t /*nSessionId*/) override { return true; }
  void onSessionClosed(uint32_t /*nSessionId*/) override {}

protected:
  // BufferedProtobufTerminal interface
  void handleMessage(uint32_t nSessionId, spex::ICommutator &&message) override;

  virtual void handleCommutatorMessage(size_t, spex::ICommutator const&) {}
  // Messages, that were incapsulated to spex::ICommutator::Message message
  virtual void handleNavigationMessage(size_t, spex::INavigation const&) {}

  void send(uint32_t nSessionId, spex::INavigation &&message);

private:
  std::string m_sModuleType;
};

using BaseModulePtr     = std::shared_ptr<BaseModule>;
using BaseModuleWeakPtr = std::weak_ptr<BaseModule>;

} // namespace modules
