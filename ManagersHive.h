#pragma once

#include <memory>

#include "Modules/Commutator/CommutatorManager.h"
#include "Ships/ShipsManager.h"

// All managers are gethered toghether here, in managers hive
struct ManagersHive
{
  ManagersHive()
    : m_pCommutatorsManager(std::make_shared<modules::CommutatorManager>()),
      m_pShipsManager(std::make_shared<ships::ShipsManager>())
  {}

  modules::CommutatorManagerPtr m_pCommutatorsManager;
  ships::ShipsManagerPtr        m_pShipsManager;
};

using ManagersHivePtr = std::shared_ptr<ManagersHive>;
