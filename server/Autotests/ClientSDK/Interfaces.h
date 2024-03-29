#pragma once

#include <memory>
#include <Protocol.pb.h>
namespace autotests { namespace client {

template<typename FrameType>
class IChannel;
template<typename FrameType>
using IChannelPtr = std::shared_ptr<IChannel<FrameType>>;

template<typename FrameType>
class ITerminal;
template<typename FrameType>
using ITerminalPtr = std::shared_ptr<ITerminal<FrameType>>;

class IProceedable
{
public:
  virtual ~IProceedable() = default;

  virtual void proceed(uint32_t nDeltaUs) = 0;
  virtual bool isComplete() const = 0;
};

template<typename FrameType>
class IChannel
{
public:
  virtual ~IChannel() = default;

  virtual bool send(FrameType&& message) = 0;

  virtual void attachToTerminal(ITerminalPtr<FrameType> pTerminal) = 0;
  virtual void detachFromTerminal() = 0;
};

template<typename FrameType>
class ITerminal
{
public:
  virtual ~ITerminal() = default;

  virtual void onMessageReceived(FrameType&& message) = 0;

  virtual void attachToDownlevel(IChannelPtr<FrameType> pDownlevel) = 0;
  virtual void detachDownlevel() = 0;

};


template<typename FrameType>
using ITerminalWeakPtr = std::weak_ptr<ITerminal<FrameType>>;


using IPlayerChannel = IChannel<spex::Message>;
using IPlayerChannelPtr  = std::shared_ptr<IPlayerChannel>;

using IPlayerTerminal = ITerminal<spex::Message>;
using IPlayerTerminalPtr = std::shared_ptr<IPlayerTerminal>;

using IPlayerTerminalWeakPtr = std::weak_ptr<IPlayerTerminal>;

}}  // namespace autotests::client
