#pragma once

#include <memory>
#include <Newton/PhysicalObject.h>
#include <Modules/BaseModule.h>
#include <Utils/GlobalContainer.h>
#include <Protocol.pb.h>

namespace blueprints {
class BlueprintsLibrary;
}

namespace modules {

class BlueprintsStorage :
    public BaseModule,
    public utils::GlobalContainer<BlueprintsStorage>
{
public:
  BlueprintsStorage(world::PlayerWeakPtr pOwner);

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
  blueprints::BlueprintsLibrary const& getLibrary() const;
};

using BlueprintsStoragePtr = std::shared_ptr<BlueprintsStorage>;

} // namespace modules
