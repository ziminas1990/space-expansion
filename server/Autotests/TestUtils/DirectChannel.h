#pragma once

#include <Network/Interfaces.h>

namespace autotests {

template<typename FrameType>
class DirectChannel : public network::IChannel<FrameType>
{
  // This class just receives messages as IChannel<FrameType> and pass them
  // to the attached ITermonal<FrameType>
private:
  network::ITerminalPtr<FrameType> m_pTerminal;

public:
  DirectChannel() = default;

  bool openSession(uint32_t nSessionId) {
    return m_pTerminal->openSession(nSessionId);
  }

  bool send(uint32_t nSessionId, FrameType const& frame) const override {
    if (m_pTerminal) {
      m_pTerminal->onMessageReceived(nSessionId, frame);
    }
    return nullptr != m_pTerminal;
  }

  void closeSession(uint32_t nSessionId) override {
    m_pTerminal->onSessionClosed(nSessionId);
  }

  bool isValid() const override { return true; }

  void attachToTerminal(network::ITerminalPtr<FrameType> pTerminal) override {
    m_pTerminal = pTerminal;
  }

  void detachFromTerminal() override {
    if (m_pTerminal) {
      m_pTerminal->detachFromChannel();
      m_pTerminal = nullptr;
    }
  }
};

using DirectPlayerChannel = DirectChannel<spex::Message>;
using DirectPlayerChannelPtr = std::shared_ptr<DirectPlayerChannel>;

} // namespace autotests
