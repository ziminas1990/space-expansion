#include <Autotests/Modules/ModulesTestFixture.h>

#include <Network/SessionMux.h>
#include <Utils/GlobalContainerUtils.h>
#include <Autotests/ClientSDK/RootSession.h>

#include "Helper.h"

namespace autotests {

void ModulesTestFixture::SetUp()
{
  std::stringstream problem;
  ASSERT_TRUE(utils::GlobalContainerUtils::checkAllContainersAreEmpty(problem))
    << problem.str();

  m_clock.switchToDebugMode();
  m_clock.setDebugTickUs(5000);       // 5ms per tick
  m_clock.proceedRequest(0xFFFFFFFF);  // As long as it is required
  m_clock.start(true);
  utils::GlobalClock::set(&m_clock);

  m_pGrid = createGlobalGrid();
  if (m_pGrid) {
    world::Grid::setGlobal(m_pGrid.get());
  }

  m_pNewtonEngine = std::make_shared<FreezableLogic>(
                                      std::make_shared<newton::NewtonEngine>());
  m_pCommutatorManager     = std::make_shared<modules::CommutatorManager>();
  m_pEngineManager         = std::make_shared<modules::EngineManager>();
  m_pPassiveScannerManager = std::make_shared<modules::PassiveScannerManager>();
  m_pMessangerManager      = std::make_shared<modules::MessangerManager>();

  m_conveyor.addLogicToChain(m_pMessangerManager);
  m_conveyor.addLogicToChain(m_pNewtonEngine);
  m_conveyor.addLogicToChain(m_pCommutatorManager);
  m_conveyor.addLogicToChain(m_pEngineManager);
  m_conveyor.addLogicToChain(m_pPassiveScannerManager);

  m_fConveyorProceeder = [this]() { this->proceedEnviroment(); };

  // Components on server
  m_pPlayer = world::Player::makeDummy("Player-1");

  // Components on client
  m_pRouter = std::make_shared<client::Router>();
  m_pRouter->setProceeder(m_fConveyorProceeder);

  m_pConnector = std::make_shared<PlayerConnector>();
  m_linker.link(m_pConnector,
                m_pPlayer->getSessionMux()->asTerminal(),
                m_pRouter);

  // Add a custom unlinker logic, that closes all active connections, otherwise
  // SessionMux will raise an assert error in it's descructor
  m_linker.addCustomUnlinker([this]() {
    m_pPlayer->getSessionMux()->markAllConnectionsAsClosed();
  });
}

} // namespace autotests