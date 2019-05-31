#pragma once

#include <string>
#include <memory>
#include "Ships/ShipBlueprint.h"

#include <Utils/YamlForwardDeclarations.h>

namespace blueprints
{

class BlueprintsStorage
{
public:
  bool loadBlueprints(YAML::Node const& data);

  ships::ShipBlueprintConstPtr getBlueprint(std::string const& sShipType) const;

private:
  std::map<std::string, ships::ShipBlueprintPtr> m_blueprints;
};

using BlueprintsStoragePtr = std::shared_ptr<BlueprintsStorage>;

} // namespace blueprints
