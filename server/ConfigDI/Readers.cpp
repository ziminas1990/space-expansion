#include "Readers.h"
#include <Utils/YamlReader.h>
#include <yaml-cpp/yaml.h>

namespace config
{

PortsPoolCfg PortsPoolCfgReader::read(YAML::Node const& data)
{
  uint16_t begin = 0;
  uint16_t end   = 0;
  if (!utils::YamlReader(data).read("begin", begin).read("end", end).isOk()) {
    return PortsPoolCfg();
  }

  return PortsPoolCfg()
      .setBegin(begin)
      .setEnd(end);
}

GlobalGridCfg GlobalGridCfgReader::read(const YAML::Node &data)
{
  uint8_t  nGridSize;
  uint16_t nCellWidthKm;
  if (!utils::YamlReader(data)
      .read("grid-size", nGridSize)
      .read("cell-width-km", nCellWidthKm)
      .isOk()) {
    return GlobalGridCfg();
  }
  return GlobalGridCfg()
      .setGridSize(nGridSize)
      .setCellWidthKm(nCellWidthKm);
}

AdministratorCfg AdministratorCfgReader::read(YAML::Node const& data)
{
  uint16_t    nPort;
  std::string sLogin;
  std::string sPassword;
  if (!utils::YamlReader(data)
      .read("udp-port", nPort)
      .read("login", sLogin)
      .read("password", sPassword)
      .isOk()) {
    return AdministratorCfg();
  }

  return AdministratorCfg()
      .setPort(nPort)
      .setLogin(sLogin)
      .setPassord(sPassword);
}

ApplicationCfg ApplicationCfgReader::read(YAML::Node const& data)
{
  uint16_t    nTotalThreads = 0;
  uint16_t    nLoginUdpPort = 0;
  std::string sInitialState;
  if (!utils::YamlReader(data)
      .read("total-threads", nTotalThreads)
      .read("login-udp-port", nLoginUdpPort)
      .read("initial-state", sInitialState)
      .isOk()) {
    return ApplicationCfg();
  }

  bool isClockFreezed = sInitialState == "freezed";

  return ApplicationCfg()
      .setLoginUdpPort(nLoginUdpPort)
      .setTotalThreads(nTotalThreads)
      .setAdministratorCfg(
        AdministratorCfgReader::read(data["administrator"]))
      .setClockInitialState(isClockFreezed)
      .setPortsPool(
        PortsPoolCfgReader::read(data["ports-pool"]))
      .setGlobalGrid(
        GlobalGridCfgReader::read(data["administrator"]));
}

} // namespace config
