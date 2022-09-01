#pragma once

#include <Protocol.pb.h>

namespace utils {

template<typename FrameType>
constexpr bool isPlayerMessage(const FrameType&)
{
  return std::is_same_v<FrameType, spex::Message>;
}

template<typename FrameType>
bool isHeartbeat(const FrameType& frame) {
  if constexpr (isPlayerMessage(frame)) {
    const spex::Message& message = frame;
    return message.choice_case() == spex::Message::kSession 
        && message.session().choice_case() == spex::ISessionControl::kHeartbeat;
  }
  return false;
}

} // namespace utils

