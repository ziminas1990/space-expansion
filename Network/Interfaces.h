#pragma once

#include <stdint.h>
#include <memory>

#include <Protocol.pb.h>

namespace network
{

using MessagePtr = uint8_t const*;

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

  virtual bool sendMessage(MessagePtr pMessage, size_t nLength) = 0;
};


class IProtobufTerminal
{
public:
  virtual ~IProtobufTerminal() = default;

  virtual void onMessageReceived(spex::CommandCenterMessage const& message) = 0;
};


class IProtobufChannel
{
public:
  virtual ~IProtobufChannel() = default;

  virtual void sendMessage(spex::CommandCenterMessage const& message) = 0;
};

using ITerminalPtr         = std::shared_ptr<ITerminal>;
using IChannelPtr          = std::shared_ptr<IChannel>;
using IProtobufTerminalPtr = std::shared_ptr<IProtobufTerminal>;
using IProtobufChannelPtr  = std::shared_ptr<IProtobufChannel>;

} // namespace network
