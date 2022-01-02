#pragma once

#include <stdint.h>
#include <iostream>
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

  bool isValid(std::ostream& problem) const {
    const char* prefix = "Wrong ports pool configuration: ";
    if (!m_nBegin || !m_nEnd) {
      problem << prefix << "invalid ports pool bounds: " <<
                 m_nBegin << " to " << m_nEnd;
      return false;
    }
    if (m_nEnd <= m_nBegin) {
      problem << prefix << "right bound (" << m_nEnd <<
                 ") must be greater than left bound (" << m_nBegin << ")";
      return false;
    }
    return true;
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

  bool isValid(std::ostream& problem) const {
    const char* prefix = "wrong global grid configuration: ";
    if (m_nCellWidthKm == 0) {
      problem << prefix << "cell width must be greater than 0";
      return false;
    }
    if (m_nGridSize == 0 || m_nGridSize > 300) {
      problem << prefix << "grid size value must be between (0, 300)";
      return false;
    }
    return true;
  }

  GlobalGridCfg& setGridSize(uint16_t nGridSize);
  GlobalGridCfg& setCellWidthKm(uint16_t nCellWidthKm);

  uint16_t gridSize()    const override { return m_nGridSize; }
  uint16_t cellWidthKm() const override { return m_nCellWidthKm; }

private:
  uint16_t m_nCellWidthKm;
  uint16_t m_nGridSize;
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

  bool isValid(std::ostream& problem) const {
    const char* prefix = "Wrong administrator configuration: ";
    if (m_nPort == 0) {
      problem << prefix << "listen port can't be 0";
      return false;
    }
    if (m_sLogin.empty()) {
      problem << prefix << "login can't be empty";
      return false;
    }
    if (m_sPassword.empty()) {
      problem << prefix << "password can't be empty";
      return false;
    }
    return true;
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

  bool isValid(std::ostream& problem) const {
    const char* prefix = "Wrong app general configuration: ";
    if (!m_nTotalThreads) {
      problem << prefix << "total thread must be greater than 0";
      return false;
    }
    if (!m_nLoginUdpPort) {
      problem << prefix << "login port must be greater than 0";
      return false;
    }
    return m_portsPool.isValid(problem)
        && m_globalGrid.isValid(problem);
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
