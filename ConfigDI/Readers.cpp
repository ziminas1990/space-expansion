#include "Readers.h"
#include <Utils/YamlReader.h>
#include <yaml-cpp/yaml.h>

namespace config
{

PortsPoolCfg PortsPoolCfgReader::read(YAML::Node const& data)
{
  uint16_t begin = 0;
  uint16_t end   = 0;
  if (utils::YamlReader(data).read("begin", begin).read("end", end).isOk())
    return PortsPoolCfg()
        .setBegin(begin)
        .setEnd(end);
  return PortsPoolCfg();
}

ApplicationCfg ApplicationCfgReader::read(YAML::Node const& data)
{
  uint16_t nTotalThreads = 0;
  uint16_t nLoginUdpPort = 0;
  if (!utils::YamlReader(data)
      .read("total-threads", nTotalThreads)
      .read("login-udp-port", nLoginUdpPort)
      .isOk()) {
    return ApplicationCfg();
  }

  return ApplicationCfg()
      .setLoginUdpPort(nLoginUdpPort)
      .setTotalThreads(nTotalThreads)
      .setPortsPool(
        PortsPoolCfgReader::read(data["ports-pool"]));
}

} // namespace config
