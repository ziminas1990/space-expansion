#pragma once

#include <stdint.h>

namespace config
{

class IPortsPoolCfg
{
public:
  virtual ~IPortsPoolCfg() = default;

  virtual uint16_t begin() const = 0;
  virtual uint16_t end()   const = 0;
};


class IApplicationCfg
{
public:
  virtual ~IApplicationCfg() = default;

  virtual uint16_t             getTotalThreads() const = 0;
  virtual uint16_t             getLoginUdpPort() const = 0;
  virtual IPortsPoolCfg const& getPortsPoolcfg() const = 0;

};

} // namespace config
