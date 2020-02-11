#pragma once

#include <stdint.h>
#include <vector>

#include <Autotests/ClientSDK/Interfaces.h>
#include <Autotests/ClientSDK/ClientBaseModule.h>
#include <World/Resources.h>

namespace autotests { namespace client {

class BlueprintName
{
public:
  BlueprintName(std::string const& sName);

  bool operator<(BlueprintName const& other) const {
    return m_sModuleClass != other.m_sModuleClass
        ? m_sModuleClass < other.m_sModuleClass
        : m_sModuleType  < other.m_sModuleType;
  }

  std::string const& getClass()    const { return m_sModuleClass; }
  std::string const& getType()     const { return m_sModuleType; }
  std::string        getFullName() const {
    return m_sModuleClass + "/" + m_sModuleType;
  }

private:
  std::string m_sModuleClass;
  std::string m_sModuleType;
};

struct Property;
using PropertyUniqPtr = std::unique_ptr<Property>;
using Properties = std::map<std::string, PropertyUniqPtr>;

struct Property {
  std::string sName;
  std::string sValue;
  Properties  nested;
};

struct Blueprint {
  std::string      m_sName;
  Properties       m_properties;
  world::Resources m_expenses;
};


class BlueprintsStorage : public ClientBaseModule
{
public:
  enum Status {
    eSuccess,

    // Client SDK errors
    eTransportError,
    eUnexpectedMessage,

    // Errors from server
    eBlueprintNotFound,
    eInternalError,
  };

  bool getModulesBlueprintsNames(std::string const& sFilter,
                                 std::vector<BlueprintName> &output);

  Status getBlueprint(BlueprintName const& name, Blueprint& out);

};

using BlueprintsStoragePtr = std::shared_ptr<BlueprintsStorage>;

}}  // namespace autotests::client
