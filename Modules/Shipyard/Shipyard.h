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
#include <Modules/ResourceContainer/ResourceContainer.h>

namespace modules {

class Shipyard :
    public BaseModule,
    public utils::GlobalContainer<Shipyard>
{
public:
  Shipyard(std::string&& sName, world::PlayerWeakPtr pOwner,
           double laborPerSecond, std::string sContainerName);

  // override from BaseModule
  bool loadState(YAML::Node const& data) override;
  void proceed(uint32_t nIntervalUs) override;

  bool openSession(uint32_t nSessionId) override;
  void onSessionClosed(uint32_t nSessionId) override;

private:
  void handleShipyardMessage(
      uint32_t nTunnelId, spex::IShipyard const& message) override;

  void finishBuildingProcedure();

  void sendSpeification(uint32_t nSessionId);
  void sendBuildStatus(spex::IShipyard::Status eStatus);
  void sendBuildProgress(double progress);
  void sendBuildComplete(std::string &&sShipName, uint32_t nSlotId);

private:
  double m_laborPerSecond;
    // Shipyard efficency (how many labor it produces in second)
  std::string m_sContainerName;

  std::set<uint32_t> m_openedSessions;

  struct BuildingTask {
    BuildingTask() : progress(0) {}

    blueprints::ShipBlueprintPtr  pShipBlueprint;
    std::string                   sShipName;
    double                        progress; 
    modules::ResourceContainerPtr pContainer;
      // Container, which resources will be consumed
    world::ResourcesArray         resources;
      // How many resources should be consumed to build item
    blueprints::BlueprintsLibrary localLibraryCopy;
      // When building proedures starts, a copy of blueprints library will be
      // created. It is neccessary, because library can be changed, while ship is
      // being built and that would lead to undefined behavior (I mean not cpp ub!).
    uint32_t                      nIntervalSinceLastProgressInd;
      // Stores a number of usec, that passed after previos progress IND was sent
  } m_building;
};

using ShipyardPtr = std::shared_ptr<Shipyard>;

} // namespace modules
