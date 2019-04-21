#pragma once

#include <memory>
#include <Network/BufferedTerminal.h>
#include <Network/ConnectionManager.h>

namespace modules {

class AccessPanel : public network::BufferedTerminal
{
public:
  void attachToConnectionManager(network::ConnectionManagerPtr pManager)
  { m_pConnectionManager = pManager; }

protected:
  // overrides from BufferedTerminal interface
  void handleMessage(network::MessagePtr pMessage, size_t nLength) override;

private:
  bool checkLogin(std::string const& sLogin, std::string const& nPassword);

  void sendLoginSuccess(network::UdpEndPoint const& localAddress);
  void sendLoginFailed(std::string const& reason);

private:
  network::ConnectionManagerPtr m_pConnectionManager;
};

using AccessPanelPtr = std::shared_ptr<AccessPanel>;


class AccessPanelFacotry : public network::IBufferedTerminalFactory
{
public:

  void setCreationData(network::ConnectionManagerPtr pManager)
  {
    m_pManager = pManager;
  }

  // overrides from IBufferedTerminalFactory interface
  network::BufferedTerminalPtr make()
  {
    AccessPanelPtr pPanel = std::make_shared<AccessPanel>();
    pPanel->attachToConnectionManager(m_pManager);
    return pPanel;
  }

private:
  network::ConnectionManagerPtr m_pManager;
};

using AccessPanelFacotryPtr = std::shared_ptr<AccessPanelFacotry>;

} // namespace modules
