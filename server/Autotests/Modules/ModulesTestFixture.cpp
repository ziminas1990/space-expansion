#include <Autotests/Modules/ModulesTestFixture.h>

namespace autotests {

void ModulesTestFixture::SetUp()
{
  m_clock.switchToDebugMode();
  m_clock.setDebugTickUs(25000);       // 25ms per tick
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

  m_conveyor.addLogicToChain(m_pNewtonEngine);
  m_conveyor.addLogicToChain(m_pCommutatorManager);
  m_conveyor.addLogicToChain(m_pEngineManager);
  m_conveyor.addLogicToChain(m_pPassiveScannerManager);

  m_fConveyorProceeder = [this]() { this->proceedEnviroment(); };

  // Components on server
  m_pPlayer = world::Player::makeDummy("Player-1");

  const uint32_t nConnectionId = 5;
  const uint32_t nRootSessionId = m_pPlayer->onNewConnection(nConnectionId);

  // Components on client
  m_pRouter = std::make_shared<client::Router>();
  m_pRouter->setProceeder(m_fConveyorProceeder);

  m_pConnection = std::make_shared<PlayerConnector>(nConnectionId);
  m_connectionGuard.link(m_pConnection,
                         m_pPlayer->getSessionMux()->asTerminal(),
                         m_pRouter);

  m_pCommutatorCtrl = std::make_shared<client::ClientCommutator>(m_pRouter);
  m_pCommutatorCtrl->attachToChannel(m_pRouter->openSession(nRootSessionId));
}

} // namespace autotests