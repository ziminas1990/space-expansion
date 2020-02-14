#pragma once

#include <memory>
#include <vector>

#include <Modules/BaseModule.h>
#include <Utils/YamlForwardDeclarations.h>
#include <World/Resources.h>

namespace world {
class Player;
using PlayerPtr     = std::shared_ptr<Player>;
using PlayerWeakPtr = std::weak_ptr<Player>;
}

namespace blueprints {

class BaseBlueprint;
using BaseBlueprintPtr = std::shared_ptr<BaseBlueprint>;

class BaseBlueprint
{
public:
  virtual ~BaseBlueprint() = default;

  virtual modules::BaseModulePtr build(std::string sName,
                                       world::PlayerWeakPtr pOwner) const = 0;

  virtual bool load(YAML::Node const& data);
  virtual void dump(YAML::Node& out) const;

  world::Resources const& expenses()  const { return m_expenses; }
    // Return total number of resources, that are required to produce item

private:
  world::Resources m_expenses;
};

} // namespace modules
