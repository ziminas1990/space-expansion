#pragma once

#include <optional>

#include <Modules/BaseModule.h>
#include <Utils/GlobalContainer.h>
#include <Utils/UnorderedVector.h>
#include <Protocol.pb.h>

namespace modules {

class Game : public BaseModule, public utils::GlobalObject<Game>
{
public:
  static constexpr const char* TypeName() { return "Game"; }

  Game(std::string&& sName, world::PlayerWeakPtr pOwner);

  void notifyGameOver(const spex::IGame::GameOver& report);

protected:
  void handleGameMessage(uint32_t nSessionId, spex::IGame const& message) override;
  void onSessionClosed(uint32_t nSessionId) override;

private:
  void monitor(uint32_t nSessionId);
  bool sendGameOver(uint32_t nSessionId, const spex::IGame::GameOver& report) const;
  void notifyMonitors();

private:
  utils::UnorderedVector<uint32_t> m_monitoringSessions;
  std::optional<spex::IGame::GameOver> m_gameOver;
};

} // namespace modules
