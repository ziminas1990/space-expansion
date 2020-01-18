#pragma once

#include <utility>
#include <string>

namespace modules {

class BlueprintName
{
public:
  BlueprintName(std::string sModuleClass, std::string sModuleType)
    : m_sModuleClass(std::move(sModuleClass)), m_sModuleType(std::move(sModuleType))
  {}

  std::string const& getModuleClass() const { return m_sModuleClass; }
  std::string const& getModuleType()  const { return m_sModuleType; }

  bool operator<(BlueprintName const& other) const {
    return (m_sModuleClass != other.m_sModuleClass)
        ? m_sModuleClass < other.m_sModuleClass
        : m_sModuleType < other.m_sModuleType;
  }

private:
  std::string m_sModuleClass;
  std::string m_sModuleType;
};

} // namespace modules
