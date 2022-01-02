#pragma once

#include <stdint.h>
#include <string>

namespace config
{

class IPortsPoolCfg
{
public:
  virtual ~IPortsPoolCfg() = default;

  virtual uint16_t begin() const = 0;
  virtual uint16_t end()   const = 0;
};

class IGlobalGridCfg
{
public:
  virtual ~IGlobalGridCfg() = default;

  virtual uint16_t gridSize()    const = 0;
  virtual uint16_t cellWidthKm() const = 0;
};

class IAdministratorCfg
{
public:
  virtual ~IAdministratorCfg() = default;

  virtual uint16_t    getPort()     const = 0;
  virtual std::string getLogin()    const = 0;
  virtual std::string getPassword() const = 0;
};

class IApplicationCfg
{
public:
  virtual ~IApplicationCfg() = default;

  virtual uint16_t                 getTotalThreads()     const = 0;
  virtual uint16_t                 getLoginUdpPort()     const = 0;
  virtual IPortsPoolCfg     const& getPortsPoolcfg()     const = 0;
  virtual IAdministratorCfg const& getAdministratorCfg() const = 0;
  virtual IGlobalGridCfg    const& getGlobalGridCfg()    const = 0;
  virtual bool                     isClockFreezed()      const = 0;
};

} // namespace config
