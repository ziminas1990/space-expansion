#pragma once

#include <memory>
#include "ProtobufSyncPipe.h"

namespace autotests {

class ProtobufTunnel : public ProtobufSyncPipe
{
public:
  ProtobufTunnel(uint32_t nSessionId)
    : m_nSessionId(nSessionId)
  {}

  // override from IProtobufTerminal
  bool send(uint32_t nSessionId, spex::Message const& frame) const override;

private:
  uint32_t m_nSessionId;
};

using ProtobufTunnelPtr = std::shared_ptr<ProtobufTunnel>;

} // namespace autotests
