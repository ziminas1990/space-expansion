#include "BaseModule.h"

namespace modules {

void BaseModule::handleMessage(uint32_t nSessionId, spex::ICommutator &&message)
{
  if (message.choice_case() != spex::ICommutator::kMessage) {
    handleCommutatorMessage(nSessionId, message);
    return;
  }

  switch(message.message().choice_case()) {
    case spex::ICommutator::Message::kNavigationMessage: {
      handleNavigationMessage(nSessionId, message.message().navigationmessage());
      return;
    }
    case spex::ICommutator::Message::kCommutator:
    case spex::ICommutator::Message::CHOICE_NOT_SET: {
      // Just ignoring
      return;
    }
  }
}

bool BaseModule::sendToClient(uint32_t nSessionId, spex::INavigation&& message) const
{
  spex::ICommutator commutatorMessage;
  *commutatorMessage.mutable_message()->mutable_navigationmessage() = std::move(message);
  return BufferedProtobufTerminal::send(nSessionId, std::move(commutatorMessage));
}

} // namespace modules
