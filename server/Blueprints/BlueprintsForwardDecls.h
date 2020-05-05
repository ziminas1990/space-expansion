#pragma once

#include <memory>

namespace blueprints {

class BaseBlueprint;
using BaseBlueprintPtr     = std::shared_ptr<BaseBlueprint>;
using BaseBlueprintWeakPtr = std::weak_ptr<BaseBlueprint>;

class BlueprintsLibrary;
using BlueprintsLibraryPtr     = std::shared_ptr<BlueprintsLibrary>;
using BlueprintsLibraryWeakPtr = std::weak_ptr<BlueprintsLibrary>;

class ShipBlueprint;
using ShipBlueprintPtr      = std::shared_ptr<ShipBlueprint>;
using ShipBlueprintConstPtr = std::shared_ptr<ShipBlueprint const>;

} // namespace blueprints
