#include "ProtobufTunnel.h"

namespace autotests
{

bool ProtobufTunnel::send(uint32_t nSessionId, spex::Message const& frame) const
{
  spex::Message message;
  message.mutable_encapsulated()->CopyFrom(frame);
  message.set_tunnelid(nSessionId);
  return ProtobufSyncPipe::send(m_nSessionId, message);
}

} // namespace autotests
