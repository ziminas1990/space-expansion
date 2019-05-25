#pragma once

#include <memory>
#include "Ship.h"
#include <Modules/ModuleBlueprint.h>

namespace ships {

class ShipBlueprint;
using ShipBlueprintPtr = std::shared_ptr<ShipBlueprint>;

class ShipBlueprint
{
public:
  virtual ~ShipBlueprint() = default;

  virtual ShipPtr build() const;

  virtual ShipBlueprintPtr wrapToSharedPtr()
  {
    return std::make_shared<ShipBlueprint>(std::move(*this));
  }

  ShipBlueprint& setWeight(double weight);
  ShipBlueprint& setShipType(std::string sShipType);
  ShipBlueprint& addModule(modules::ModuleBlueprintPtr pModuleBlueprint);

private:
  std::string m_sShipType  = "unknown";
  double      m_shipWeight = 1000;

  std::vector<modules::ModuleBlueprintPtr> m_modules;
};


class BlueprintsStore
{
public:
  static ShipBlueprintPtr makeCommandCenterBlueprint();
  static ShipBlueprintPtr makeCorvetBlueprint();
  static ShipBlueprintPtr makeMinerBlueprint();
  static ShipBlueprintPtr makeZondBlueprint();
};

} // namespace ships
