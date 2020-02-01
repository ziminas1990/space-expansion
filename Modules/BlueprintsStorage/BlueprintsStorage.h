#pragma once

#include <memory>
#include <Newton/PhysicalObject.h>
#include <Modules/BaseModule.h>
#include <Utils/GlobalContainer.h>
#include <Protocol.pb.h>
#include <Blueprints/Modules/BlueprintsLibrary.h>
#include <Blueprints/Ships/ShipBlueprintsLibrary.h>

namespace modules {

class BlueprintsStorage :
    public BaseModule,
    public utils::GlobalContainer<BlueprintsStorage>
{
public:
  BlueprintsStorage();

  void attachToLibraries(BlueprintsLibrary const* pModulesBlueprints,
                         ships::ShipBlueprintsLibrary const* pShipsBlueprints);

protected:
  // override from BaseModule
  void handleBlueprintsStorageMessage(
      uint32_t nSessionId,
      spex::IBlueprintsLibrary const& message) override;

  void onModulesListReq(uint32_t nSessionId, std::string const& sFilter) const;
  void onModuleBlueprintReq(uint32_t nSessionId, std::string const& sName) const;

  bool sendModuleBlueprintFail(uint32_t nSessionId,
                               spex::IBlueprintsLibrary::Status error) const;
private:
  BlueprintsLibrary            const* m_pModulesBlueprints;
  ships::ShipBlueprintsLibrary const* m_pShipsBlueprints;
};

using BlueprintsStoragePtr = std::shared_ptr<BlueprintsStorage>;

} // namespace modules
