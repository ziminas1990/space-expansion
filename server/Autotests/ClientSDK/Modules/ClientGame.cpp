#include "ClientGame.h"

namespace autotests { namespace client {

bool Game::monitor()
{
  spex::Message request;
  request.mutable_game()->set_monitor(true);
  return send(std::move(request));
}

bool Game::waitGameOverReport(spex::IGame::GameOver& report, uint16_t nTimeout)
{
  spex::IGame message;
  if (!wait(message, nTimeout)) {
    return false;
  }
  if (message.choice_case() != spex::IGame::kGameOverReport) {
    return false;
  }
  report = std::move(*message.mutable_game_over_report());
  return true;
}

}}  // namespace autotests::client
