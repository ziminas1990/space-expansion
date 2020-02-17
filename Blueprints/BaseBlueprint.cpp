#include "BaseBlueprint.h"

#include <World/Resources.h>
#include <yaml-cpp/yaml.h>

namespace blueprints {

bool BaseBlueprint::load(YAML::Node const& data)
{
  YAML::Node const& expenses = data["expenses"];
  assert(expenses.IsDefined());
  assert(expenses.IsMap());
  if (!expenses.IsMap()) {
    return false;
  }

  m_expenses.fill(0);
  for (auto const& resource : expenses) {
    world::ResourceItem item;
    if (!item.load(resource)) {
      assert("Failed to load item" == nullptr);
      return false;
    }
    m_expenses[item.m_eType] += item.m_nAmount;
  }
  return true;
}

void BaseBlueprint::dump(YAML::Node& out) const
{
  YAML::Node expenses;
  for (size_t i = 0; i < m_expenses.size(); ++i) {
    world::ResourceItem item(static_cast<world::Resource::Type>(i), m_expenses[i]);
    item.dump(expenses);
  }
  out["expenses"] = std::move(expenses);
}

void BaseBlueprint::expenses(world::ResourcesArray& out) const
{
  for (size_t i = 0; i < m_expenses.size(); ++i) {
    out[i] += m_expenses[i];
  }
}

} // namespace modules
