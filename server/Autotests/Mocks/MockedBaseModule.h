#pragma once

#include <memory>
#include <queue>
#include <functional>
#include <Modules/BaseModule.h>

namespace autotests
{

class MockedBaseModule : public modules::BaseModule
{
  using MessagesQueue = std::queue<spex::Message>;
  using SessionsQueue = std::map<uint32_t, MessagesQueue>;

public:
  MockedBaseModule(std::string&& sModuleType = "MockedBaseModule")
    : modules::BaseModule(std::move(sModuleType), std::string(), world::PlayerWeakPtr())
  {}

  void setEnviromentProceeder(std::function<void()> fProceeder)
  { m_fEnviromentProceeder = std::move(fProceeder); }

  // Waiting message in already opened session nSessionId
  bool waitAny(uint32_t nSessionId, uint16_t nTimeoutMs = 100);
  bool waitAny(uint32_t nSessionId, spex::Message &out, uint16_t nTimeoutMs = 100);

  bool wait(uint32_t nSessionId, spex::IAccessPanel &out, uint16_t nTimeoutMs = 100);
  bool wait(uint32_t nSessionId, spex::ICommutator &out, uint16_t nTimeoutMs = 100);
  bool wait(uint32_t nSessionId, spex::INavigation &out, uint16_t nTimeoutMs = 100);

  // Expect, that no message will be received in session
  bool expectSilence(uint32_t nSessionId, uint16_t nTimeoutMs);

  // overrides from ITerminal interface
  void onMessageReceived(
      uint32_t nSessionId, spex::Message const& message) override;
  void attachToChannel(network::IPlayerChannelPtr pChannel) override;
  void detachFromChannel() override;

private:
  bool waitConcrete(uint32_t nSessionId,
                    spex::Message::ChoiceCase eExpectedChoice,
                    spex::Message &out,
                    uint16_t nTimeoutMs = 500);

private:
  network::IPlayerChannelPtr  m_pAttachedChannel;
  std::function<void()>       m_fEnviromentProceeder;
  mutable SessionsQueue       m_Sessions;
};

using MockedBaseModulePtr = std::shared_ptr<MockedBaseModule>;

} // namespace autotests
