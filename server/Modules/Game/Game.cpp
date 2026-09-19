#include "Game.h"

DECLARE_GLOBAL_CONTAINER_CPP(modules::Game);

namespace modules {

Game::Game(std::string&& sName, world::PlayerWeakPtr pOwner)
  : BaseModule(TypeName(), std::move(sName), std::move(pOwner))
{
  GlobalObject<Game>::registerSelf(this);
}

void Game::notifyGameOver(const spex::IGame::GameOver& report)
{
  m_gameOver = report;
  notifyMonitors();
}

void Game::handleGameMessage(uint32_t nSessionId, spex::IGame const& message)
{
  switch (message.choice_case()) {
    case spex::IGame::kMonitor:
      monitor(nSessionId);
      return;
    default:
      return;
  }
}

void Game::onSessionClosed(uint32_t nSessionId)
{
  m_monitoringSessions.removeFirst(nSessionId);
  BaseModule::onSessionClosed(nSessionId);
}

void Game::monitor(uint32_t nSessionId)
{
  m_monitoringSessions.push(nSessionId);
  if (m_gameOver) {
    sendGameOver(nSessionId, *m_gameOver);
  }
}

bool Game::sendGameOver(
    uint32_t nSessionId, const spex::IGame::GameOver& report) const
{
  spex::Message message;
  *message.mutable_game()->mutable_game_over_report() = report;
  return sendToClient(nSessionId, std::move(message));
}

void Game::notifyMonitors()
{
  if (!m_gameOver) {
    return;
  }
  for (size_t i = 0; i < m_monitoringSessions.size();) {
    if (!sendGameOver(m_monitoringSessions[i], *m_gameOver)) {
      m_monitoringSessions.remove(i);
    } else {
      ++i;
    }
  }
}

} // namespace modules
