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

protected:
  // BufferedProtobufTerminal interface
  void handleMessage(size_t nSessionId, spex::ICommutator &&message) override;

  virtual void handleCommutatorMessage(size_t, spex::ICommutator const&) {}
  // Messages, that were incapsulated to spex::ICommutator::Message message
  virtual void handleNavigationMessage(size_t, spex::INavigation const&) {}

  void send(size_t nSessionId, spex::INavigation &&message);

private:
  std::string m_sModuleType;
};

using BaseModulePtr     = std::shared_ptr<BaseModule>;
using BaseModuleWeakPtr = std::weak_ptr<BaseModule>;

} // namespace modules
