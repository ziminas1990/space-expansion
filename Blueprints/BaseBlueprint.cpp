#include "BaseBlueprint.h"

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

  m_expenses.reserve(expenses.size());
  for (auto const& resource : expenses) {
    world::ResourceItem item;
    if (!item.load(resource)) {
      assert("Failed to load item" == nullptr);
      return false;
    }
    m_expenses.push_back(item);
  }

  assert(expenses.size() == m_expenses.size());
  return true;
}

void BaseBlueprint::dump(YAML::Node& out) const
{
  YAML::Node expenses;
  for (world::ResourceItem resource : m_expenses) {
    resource.dump(expenses);
  }
  out["expenses"] = std::move(expenses);
}

} // namespace modules
