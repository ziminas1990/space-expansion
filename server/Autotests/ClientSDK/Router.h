#pragma once
#include <Autotests/ClientSDK/SyncPipe.h>

#include <stdint.h>
#include <map>
#include <memory>

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
        message.set_tunnelid(m_nSessionId);
        return PlayerPipe::send(std::move(message));
    }

  private:
    uint32_t m_nSessionId;
  };

  using SessionPtr = std::shared_ptr<Session>;

private:
  IPlayerChannelPtr              m_pDownLevel;
  std::map<uint32_t, SessionPtr> m_sessions;

public:

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