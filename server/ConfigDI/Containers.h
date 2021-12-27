#pragma once

#include <stdint.h>
#include "Interfaces.h"

namespace config
{

class PortsPoolCfg : public IPortsPoolCfg
{
public:
  PortsPoolCfg() : m_nBegin(0), m_nEnd(0) {}
  PortsPoolCfg(PortsPoolCfg const& other) = default;
  PortsPoolCfg(IPortsPoolCfg const& other)
    : m_nBegin(other.begin()), m_nEnd(other.end())
  {}

  bool isValid() const {
    return m_nBegin && m_nEnd && m_nEnd > m_nBegin;
  }

  PortsPoolCfg& setBegin(uint16_t nBegin);
  PortsPoolCfg& setEnd(uint16_t nEnd);

  // IPortsPoolCfg interface
  uint16_t begin() const override { return m_nBegin; }
  uint16_t end()   const override { return m_nEnd;   }

private:
  uint16_t m_nBegin;
  uint16_t m_nEnd;
};


class GlobalGridCfg : public IGlobalGridCfg
{
public:
  GlobalGridCfg() : m_nCellWidthKm(0), m_nGridSize(0) {}

  GlobalGridCfg(GlobalGridCfg const& other) = default;

  GlobalGridCfg(IGlobalGridCfg const& other)
    : m_nCellWidthKm(other.cellWidthKm())
    , m_nGridSize(other.gridSize())
  {}

  bool isValid() const {
    return m_nCellWidthKm > 0 && m_nGridSize > 0;
  }

  GlobalGridCfg& setGridSize(uint8_t nGridSize);
  GlobalGridCfg& setCellWidthKm(uint16_t nCellWidthKm);

  uint8_t  gridSize()    const override { return m_nGridSize; }
  uint16_t cellWidthKm() const override { return m_nCellWidthKm; }

private:
  uint16_t m_nCellWidthKm;
  uint8_t  m_nGridSize;
};


class AdministratorCfg : public IAdministratorCfg
{
public:
  AdministratorCfg() = default;
  AdministratorCfg(AdministratorCfg const&) = default;
  AdministratorCfg(IAdministratorCfg const& other)
    : m_nPort(other.getPort()),
      m_sLogin(other.getLogin()),
      m_sPassword(other.getPassword())
  {}

  bool isValid() const {
    return m_nPort != 0 && !m_sLogin.empty() && !m_sPassword.empty();
  }

  AdministratorCfg& setPort(uint16_t nPort);
  AdministratorCfg& setLogin(std::string sLogin);
  AdministratorCfg& setPassord(std::string sPassword);

  // overrides from IAdministratorCfg
  uint16_t    getPort()     const override { return m_nPort; }
  std::string getLogin()    const override { return m_sLogin; }
  std::string getPassword() const override { return m_sPassword; }

private:
  uint16_t    m_nPort = 0;
  std::string m_sLogin;
  std::string m_sPassword;
};

class ApplicationCfg : public IApplicationCfg
{
public:
  ApplicationCfg();
  ApplicationCfg(IApplicationCfg const& other);

  bool isValid() const {
    return m_nTotalThreads
        && m_nLoginUdpPort
        && m_portsPool.isValid()
        && m_globalGrid.isValid();
  }

  ApplicationCfg& setTotalThreads(uint16_t nTotalThreads);
  ApplicationCfg& setLoginUdpPort(uint16_t nLoginUdpPort);
  ApplicationCfg& setPortsPool(IPortsPoolCfg const& cfg);
  ApplicationCfg& setGlobalGrid(IGlobalGridCfg const& cfg);
  ApplicationCfg& setAdministratorCfg(IAdministratorCfg const& cfg);
  ApplicationCfg& setClockInitialState(bool lFreezed);

  // IApplicationCfg interface
  uint16_t              getTotalThreads()  const override { return m_nTotalThreads; }
  uint16_t              getLoginUdpPort()  const override { return m_nLoginUdpPort; }
  IPortsPoolCfg const&  getPortsPoolcfg()  const override { return m_portsPool; }
  IGlobalGridCfg const& getGlobalGridCfg() const override { return m_globalGrid; }
  bool                  isClockFreezed()   const override { return m_lIsClockFreezed; }
  AdministratorCfg const& getAdministratorCfg() const override {
    return m_administratorCfg;
  }

private:
  uint16_t         m_nTotalThreads;
  uint16_t         m_nLoginUdpPort;
  PortsPoolCfg     m_portsPool;
  GlobalGridCfg    m_globalGrid;
  bool             m_lIsClockFreezed;
  AdministratorCfg m_administratorCfg;
};

} // namespace config
