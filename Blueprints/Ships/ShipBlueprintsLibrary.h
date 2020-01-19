#pragma once

#include <string>
#include "ShipBlueprint.h"

namespace ships {

class ShipBlueprintsLibrary
{
public:

  bool loadBlueprints(YAML::Node const& shipsSection);

  ShipBlueprintPtr getBlueprint(std::string const& name) const;

private:
  std::map<std::string, ShipBlueprintPtr> m_blueprints;
};

} // namespace ships
