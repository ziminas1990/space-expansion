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

  PortsPoolCfg& setBegin(uint16_t nBegin);
  PortsPoolCfg& setEnd(uint16_t nEnd);

  // IPortsPoolCfg interface
  uint16_t begin() const override { return m_nBegin; }
  uint16_t end()   const override { return m_nEnd;   }

private:
  uint16_t m_nBegin;
  uint16_t m_nEnd;
};


class ApplicationCfg : public IApplicationCfg
{
public:
  ApplicationCfg();
  ApplicationCfg(IApplicationCfg const& other);

  ApplicationCfg& setTotalThreads(uint16_t nTotalThreads);
  ApplicationCfg& setLoginUdpPort(uint16_t nLoginUdpPort);
  ApplicationCfg& setPortsPool(IPortsPoolCfg const& cfg);

  // IApplicationCfg interface
  uint16_t             getTotalThreads() const override { return m_nTotalThreads; }
  uint16_t             getLoginUdpPort() const override { return m_nLoginUdpPort; }
  const IPortsPoolCfg& getPortsPoolcfg() const override { return m_portsPool; }

private:
  uint16_t     m_nTotalThreads;
  uint16_t     m_nLoginUdpPort;
  PortsPoolCfg m_portsPool;
};

} // namespace config
