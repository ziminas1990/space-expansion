#pragma once

#include <memory>
#include <vector>

#include <Modules/BaseModule.h>
#include <Utils/YamlForwardDeclarations.h>
#include <World/Resources.h>
#include <World/WorldForwardDecls.h>

namespace blueprints {

// Blueprints are used to create items like modules or whole ships.
// Blueprint must be immutable. Blueprint object, that has been created once, should
// be never changed.
class BaseBlueprint
{
public:
  virtual ~BaseBlueprint() = default;

  virtual modules::BaseModulePtr build(std::string sName,
                                       world::PlayerWeakPtr pOwner) const = 0;

  virtual bool load(YAML::Node const& data);
  virtual void dump(YAML::Node& out) const;

  world::ResourcesArray const& expenses() const { return m_expenses; }
    // Return total number of resources, that are required to produce item
  void expenses(world::ResourcesArray& out) const;
    // Add module expenses to the specified 'out' expenses

private:
  world::ResourcesArray m_expenses;
};

} // namespace modules
