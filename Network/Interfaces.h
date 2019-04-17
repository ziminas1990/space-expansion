#pragma once

#include <stdint.h>
#include <memory>
#include <functional>

namespace network
{

using MessagePtr = uint8_t const*;
using MessageHandler = std::function<void(MessagePtr pMessage, size_t nLength)>;

class ITerminal
{
public:
  virtual ~ITerminal() = default;

  virtual void onMessageReceived(MessagePtr pMessage, size_t nLength) = 0;
};

class IChannel
{
public:
  virtual ~IChannel() = default;

  virtual void sendMessage(MessagePtr pMessage, size_t nLength) = 0;
};

//using ITerminalPtr = std::shared_ptr

} // namespace network
