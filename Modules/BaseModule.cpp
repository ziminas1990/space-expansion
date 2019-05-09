#include "BaseModule.h"

namespace modules {

void BaseModule::handleMessage(uint32_t nSessionId, spex::Message const& message)
{
  switch(message.choice_case()) {
    case spex::Message::kCommutator: {
      handleCommutatorMessage(nSessionId, message.commutator());
      return;
    }
    case spex::Message::kNavigation: {
      handleNavigationMessage(nSessionId, message.navigation());
      return;
    }
    case spex::Message::kEncapsulated: {
      // Only commutator is able to handle such messages!
      return;
    }
    case spex::Message::CHOICE_NOT_SET: {
      // Just ignoring
      return;
    }
  }
}

bool BaseModule::sendToClient(uint32_t nSessionId, spex::ICommutator const& message) const
{
  spex::Message pdu;
  pdu.mutable_commutator()->CopyFrom(message);
  return BufferedProtobufTerminal::send(nSessionId, pdu);
}

bool BaseModule::sendToClient(uint32_t nSessionId, spex::INavigation const& message) const
{
  spex::Message pdu;
  pdu.mutable_navigation()->CopyFrom(message);
  return BufferedProtobufTerminal::send(nSessionId, pdu);
}

} // namespace modules
