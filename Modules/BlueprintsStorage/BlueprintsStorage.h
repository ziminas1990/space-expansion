#pragma once

#include <memory>
#include <Newton/PhysicalObject.h>
#include <Modules/BaseModule.h>
#include <Utils/GlobalContainer.h>
#include <Protocol.pb.h>
#include <Blueprints/BlueprintsLibrary.h>

namespace modules {

class BlueprintsStorage :
    public BaseModule,
    public utils::GlobalContainer<BlueprintsStorage>
{
public:
  BlueprintsStorage();

  void attachToLibrary(BlueprintsLibrary const* pModulesBlueprints);

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
  BlueprintsLibrary const* m_pLibrary;
};

using BlueprintsStoragePtr = std::shared_ptr<BlueprintsStorage>;

} // namespace modules
