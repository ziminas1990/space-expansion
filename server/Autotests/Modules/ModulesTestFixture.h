#pragma once

#include <gtest/gtest.h>

#include <Conveyor/Conveyor.h>
#include <Utils/Clock.h>
#include <Utils/Randomizer.h>
#include <World/Grid.h>
#include <World/Player.h>
#include <Newton/NewtonEngine.h>
#include <Geometry/Rectangle.h>
#include <Autotests/TestUtils/Connector.h>
#include <Autotests/ClientSDK/Modules/ClientCommutator.h>
#include <Autotests/TestUtils/FreezableLogic.h>
#include <Autotests/TestUtils/Connector.h>
#include <Utils/Linker.h>
#include <Modules/Managers.h>

namespace autotests {

class ModulesTestFixture : public ::testing::Test
{
  friend class Helper;

public:
  ModulesTestFixture() : m_conveyor(1) {}

  void SetUp() override;

  void TearDown() override {
    world::Grid::setGlobal(nullptr);
    utils::GlobalClock::reset();
  }

  virtual std::shared_ptr<world::Grid> createGlobalGrid()
  {
    return std::make_shared<world::Grid>(128, 100000);
  }

  void freezeNewtonEngine() { m_pNewtonEngine->freeze(); }
  void resumeNewtonEngine() { m_pNewtonEngine->resume(); }

protected:

  void proceedEnviroment() {
    m_conveyor.proceed(m_clock.getNextInterval());
  }

  uint32_t justWait(uint32_t nTimeMs) {
    const uint64_t nStopAtUs = m_clock.now() + nTimeMs * 1000;
    while (m_clock.now() < nStopAtUs) {
      proceedEnviroment();
    }
    return nTimeMs + (m_clock.now() - nStopAtUs) / 1000;
  }

protected:
  // Components on server side
  utils::Clock                      m_clock;
  conveyor::Conveyor                m_conveyor;
  std::shared_ptr<world::Grid>      m_pGrid;

  // Managers
  FreezableLogicPtr                 m_pNewtonEngine;
  modules::CommutatorManagerPtr     m_pCommutatorManager;
  modules::EngineManagerPtr         m_pEngineManager;
  modules::PassiveScannerManagerPtr m_pPassiveScannerManager;
  modules::MessangerManagerPtr      m_pMessangerManager;

  std::function<void()>             m_fConveyorProceeder;

  world::PlayerPtr                  m_pPlayer;

  // Components on client's side
  client::RouterPtr m_pRouter;

  // Connects client and server side
  PlayerConnectorPtr m_pConnector;

  // Linke MUST be a last member in this class because it should be destroyed before
  // any other members
  utils::Linker m_linker;
};

}  // namespace autotests
