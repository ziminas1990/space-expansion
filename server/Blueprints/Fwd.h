#pragma once

#include <memory>

#define COMPONENT_FWD_DECLARATION(Component) \
  class Component; \
  using Component##Ptr = std::shared_ptr<Component>; \
  using Component##WeakPtr = std::weak_ptr<Component>; \

namespace blueprints {

COMPONENT_FWD_DECLARATION(BaseBlueprint)
COMPONENT_FWD_DECLARATION(BlueprintsLibrary)
COMPONENT_FWD_DECLARATION(ShipBlueprint)

}   // namespace network