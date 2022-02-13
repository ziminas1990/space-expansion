#include "Containers.h"

namespace config
{

//==============================================================================
// PortsPoolCfg
//==============================================================================

PortsPoolCfg& PortsPoolCfg::setBegin(uint16_t nBegin)
{
  m_nBegin = nBegin;
  return *this;
}

PortsPoolCfg& PortsPoolCfg::setEnd(uint16_t nEnd)
{
  m_nEnd = nEnd;
  return *this;
}

//==============================================================================
// GlobalGridCfg
//==============================================================================

bool GlobalGridCfg::isValid(std::ostream &problem) const {
  const char* prefix = "wrong global grid configuration: ";
  if (m_nCellWidthKm < 1 || m_nCellWidthKm > 1000000) {
    problem << prefix << "grid size value must be between [1, 1000000]";
    return false;
  }
  if (m_nGridSize == 0 || m_nGridSize > 300) {
    problem << prefix << "grid size value must be between (0, 300)";
    return false;
  }
  return true;
}

GlobalGridCfg& GlobalGridCfg::setGridSize(uint16_t nGridSize)
{
  m_nGridSize = nGridSize;
  return *this;
}

GlobalGridCfg& GlobalGridCfg::setCellWidthKm(uint16_t nCellWidthKm)
{
  m_nCellWidthKm = nCellWidthKm;
  return *this;
}

//==============================================================================
// AdministratorCfg
//==============================================================================

AdministratorCfg& AdministratorCfg::setPort(uint16_t nPort)
{
  m_nPort = nPort;
  return *this;
}

AdministratorCfg& AdministratorCfg::setLogin(std::string sLogin)
{
  m_sLogin = std::move(sLogin);
  return *this;
}

AdministratorCfg& AdministratorCfg::setPassord(std::string sPassword)
{
  m_sPassword = std::move(sPassword);
  return *this;
}

//==============================================================================
// ApplicationCfg
//==============================================================================

ApplicationCfg::ApplicationCfg()
  : m_nTotalThreads(1), m_nLoginUdpPort(0xFFFF)
{}

ApplicationCfg::ApplicationCfg(IApplicationCfg const& other)
  : m_nTotalThreads(other.getTotalThreads()),
    m_nLoginUdpPort(other.getLoginUdpPort()),
    m_seed(other.getSeed()),
    m_portsPool(other.getPortsPoolcfg()),
    m_globalGrid(other.getGlobalGridCfg()),
    m_lIsClockFreezed(other.isClockFreezed()),
    m_administratorCfg(other.getAdministratorCfg())
{}

ApplicationCfg& ApplicationCfg::setTotalThreads(uint16_t nTotalThreads)
{
  m_nTotalThreads = nTotalThreads;
  return *this;
}

ApplicationCfg &ApplicationCfg::setLoginUdpPort(uint16_t nLoginUdpPort)
{
  m_nLoginUdpPort = nLoginUdpPort;
  return *this;
}

ApplicationCfg &ApplicationCfg::setPortsPool(IPortsPoolCfg const& cfg)
{
  m_portsPool = cfg;
  return *this;
}

ApplicationCfg &ApplicationCfg::setGlobalGrid(const IGlobalGridCfg &cfg)
{
  m_globalGrid = cfg;
  return *this;
}

ApplicationCfg &ApplicationCfg::setAdministratorCfg(IAdministratorCfg const& cfg)
{
  m_administratorCfg = cfg;
  return *this;
}

ApplicationCfg &ApplicationCfg::setClockInitialState(bool lFreezed)
{
  m_lIsClockFreezed = lFreezed;
  return *this;
}

} // namespace config
