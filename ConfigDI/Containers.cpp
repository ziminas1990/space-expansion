#include "Containers.h"

namespace config
{

//========================================================================================
// PortsPoolCfg
//========================================================================================

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

//========================================================================================
// ApplicationCfg
//========================================================================================

ApplicationCfg::ApplicationCfg()
  : m_nTotalThreads(1), m_nLoginUdpPort(0xFFFF)
{}

ApplicationCfg::ApplicationCfg(const IApplicationCfg &other)
  : m_nTotalThreads(other.getTotalThreads()),
    m_nLoginUdpPort(other.getLoginUdpPort()),
    m_portsPool(other.getPortsPoolcfg())
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

} // namespace config
