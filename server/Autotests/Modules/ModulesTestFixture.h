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
#include <Modules/Commutator/CommutatorManager.h>
#include <Modules/PassiveScanner/PassiveScannerManager.h>

namespace autotests {

class ModulesTestFixture : public ::testing::Test
{
public:
  ModulesTestFixture() : m_conveyor(1) {}

  void SetUp() override;

  void TearDown() override {
    m_pClientCommutator->detachChannel();
      utils::GlobalClock::reset();
      // Destructor will destroy 'm_connectionGuard' object, that will unlink
      // the rest of the components
  }

private:
  void proceedEnviroment() {
    m_conveyor.proceed(m_clock.getNextInterval());
  }

protected:

  void justWait(uint32_t nTimeMs) {
    const uint64_t nStopAtUs = m_clock.now() + nTimeMs * 1000;
    while (m_clock.now() < nStopAtUs) {
      proceedEnviroment();
    }
  }

protected:
  // Components on server side
  utils::Clock                      m_clock;
  conveyor::Conveyor                m_conveyor;
  modules::CommutatorManagerPtr     m_pCommutatorManager;
  newton::NewtonEnginePtr           m_pNewtonEngine;
  modules::PassiveScannerManagerPtr m_pPassiveScannerManager;

  std::function<void()>             m_fConveyorProceeder;

  world::PlayerPtr                  m_pPlayer;

  // Component, that connects client and server sides
  PlayerConnectorPtr   m_pConnection;
  PlayerConnectorGuard m_connectionGuard;

  // Components on client's side
  client::RouterPtr           m_pRouter;
  client::ClientCommutatorPtr m_pClientCommutator;
};

}  // namespace autotests
