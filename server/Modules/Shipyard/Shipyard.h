#pragma once

#include <memory>
#include <vector>
#include <atomic>

#include <Modules/BaseModule.h>
#include <Utils/GlobalContainer.h>
#include <Utils/YamlForwardDeclarations.h>
#include <Blueprints/BlueprintsForwardDecls.h>
#include <World/Resources.h>
#include <Blueprints/BlueprintsLibrary.h>
#include <Modules/Fwd.h>


namespace modules {

class Shipyard :
    public BaseModule,
    public utils::GlobalObject<Shipyard>
{
public:
  static std::string const& TypeName() {
    const static std::string sTypeName = "Shipyard";
    return sTypeName;
  }

  Shipyard(std::string&& sName,
           world::PlayerWeakPtr pOwner,
           double laborPerSecond);

  // override from BaseModule
  bool loadState(YAML::Node const& data) override;
  void proceed(uint32_t nIntervalUs) override;

private:
  void handleShipyardMessage(
      uint32_t nTunnelId, spex::IShipyard const& message) override;

  void finishBuildingProcedure();

  void bindToCargo(uint32_t nSessionId, std::string const& name);
  void startBuildReq(uint32_t nSessionId, const spex::IShipyard::StartBuild &req);
  void cancelBuildReq(uint32_t nSessionId);

  void sendStatus(uint32_t nSessionId, spex::IShipyard::Status eStatus) const;
  void sendSpeification(uint32_t nSessionId);
  void sendBuildingReport(spex::IShipyard::Status eStatus, double progress);
  void sendBuildingReport(uint32_t nSessionId,
                          spex::IShipyard::Status eStatus,
                          double progress);
  void sendBuildComplete(std::string &&sShipName, uint32_t nSlotId);

private:
  double m_laborPerSecond;
    // Shipyard efficency (how much labor it produces per second)

  modules::ResourceContainerPtr m_pContainer;
    // Container, which resources will be consumed during the build

  struct BuildingTask {
    BuildingTask() : progress(0), nIntervalSinceLastInd(0) {}

    blueprints::ShipBlueprintPtr  pShipBlueprint;
    std::string                   sShipName;
    double                        progress; 
    world::ResourcesArray         resources;
      // How many resources should be consumed to build item
    blueprints::BlueprintsLibrary localLibraryCopy;
      // When building proedures starts, a copy of blueprints library will be
      // created. It is neccessary, because library can be changed, while ship is
      // being built and that would lead to undefined behavior (I mean not a cpp ub!).
    uint32_t                      nIntervalSinceLastInd;
      // Stores a number of usec, that passed after previos progress IND was sent
  } m_building;
};

} // namespace modules
