#pragma once
#include <Autotests/ClientSDK/SyncPipe.h>

#include <stdint.h>
#include <map>
#include <memory>
#include <functional>

namespace autotests::client {

class Router : public IPlayerTerminal
{
public:
  class Session : public PlayerPipe
  {
  public:
    Session(uint32_t nSessionId) 
      : PlayerPipe()
      , m_nSessionId(nSessionId)
    {}

    uint32_t sessionId() const { return m_nSessionId; }

    // overrides from SyncPipe
    bool send(spex::Message&& message) override {
        // std::cout << "Send to " << (m_nSessionId >> 16) << ":\n" <<
        //              message.DebugString() << std::endl;
        message.set_tunnelid(m_nSessionId);
        return PlayerPipe::send(std::move(message));
    }

    bool waitCloseTunnelInd() {
      spex::ISessionControl indication;
      return wait(indication)
          && spex::ISessionControl::kClosedInd == indication.choice_case();
    }

  private:
    uint32_t m_nSessionId;
  };

  using SessionPtr = std::shared_ptr<Session>;

private:
  IPlayerChannelPtr              m_pDownLevel;
  std::map<uint32_t, SessionPtr> m_sessions;
  std::function<void()>          m_fProceeder;

public:

  void setProceeder(std::function<void()> fProceeder) {
    m_fProceeder = std::move(fProceeder);
  }

  SessionPtr openSession(uint32_t nSessionId);
  bool closeSession(uint32_t nSessionId);

  void onMessageReceived(spex::Message&& message) override;

  void attachToDownlevel(IPlayerChannelPtr pDownlevel) override {
    m_pDownLevel = pDownlevel;
  }

  void detachDownlevel() override {
    m_pDownLevel = nullptr;
  }

};

using RouterPtr = std::shared_ptr<Router>;

} //namespace autotests::client