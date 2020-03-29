#pragma once

#include <stdint.h>
#include <memory>

#include <Protocol.pb.h>
#include <Priveledeged.pb.h>

namespace network
{

template<typename FrameType> class ITerminal;
template<typename FrameType>
using ITerminalPtr = std::shared_ptr<ITerminal<FrameType>>;

template<typename FrameType> class IChannel;
template<typename FrameType>
using IChannelPtr  = std::shared_ptr<IChannel<FrameType>>;


template<typename FrameType>
class IChannel
{
public:
  virtual ~IChannel() = default;

  virtual bool send(uint32_t nSessionId, FrameType const& frame) const = 0;
  virtual void closeSession(uint32_t nSessionId) = 0;

  virtual bool isValid() const = 0;

  virtual void attachToTerminal(ITerminalPtr<FrameType> pTerminal) = 0;
  virtual void detachFromTerminal() = 0;
};


template<typename FrameType>
class ITerminal
{
public:
  virtual ~ITerminal() = default;

  virtual bool openSession(uint32_t nSessionId) = 0;
  virtual void onMessageReceived(uint32_t nSessionId, FrameType const& frame) = 0;
  virtual void onSessionClosed(uint32_t nSessionId) = 0;

  virtual void attachToChannel(IChannelPtr<FrameType> pChannel) = 0;
  virtual void detachFromChannel() = 0;
};


// This class is just a container for message body
// It doesn't own message and should NOT release it!
struct BinaryMessage
{
  BinaryMessage() = default;
  BinaryMessage(BinaryMessage const& other) = delete;
  BinaryMessage(uint8_t const* pBody, size_t nLength)
    : m_pBody(pBody), m_nLength(nLength)
  {}
  BinaryMessage(char const* pBody, size_t nLength)
    : m_pBody(reinterpret_cast<uint8_t const*>(pBody)), m_nLength(nLength)
  {}

  uint8_t const* m_pBody   = nullptr;
  size_t         m_nLength = 0;
};


using IBinaryChannel             = IChannel<BinaryMessage>;
using IBinaryChannelPtr          = std::shared_ptr<IBinaryChannel>;
using IBinaryChannelWeakPtr      = std::weak_ptr<IBinaryChannel>;

using IBinaryTerminal            = ITerminal<BinaryMessage>;
using IBinaryTerminalPtr         = std::shared_ptr<IBinaryTerminal>;
using IBinaryTerminalWeakPtr     = std::weak_ptr<IBinaryTerminal>;

using IPlayerChannel             = IChannel<spex::Message>;
using IPlayerChannelPtr          = std::shared_ptr<IPlayerChannel>;
using IPlayerChannelWeakPtr      = std::weak_ptr<IPlayerChannel>;

using IPrivilegedChannel         = IChannel<admin::Message>;
using IPrivilegedChannelPtr      = std::shared_ptr<IPlayerChannel>;
using IPrivilegedChannelWeakPtr  = std::weak_ptr<IPlayerChannel>;

using IPlayerTerminal            = ITerminal<spex::Message>;
using IPlayerTerminalPtr         = std::shared_ptr<IPlayerTerminal>;
using IPlayerTerminalWeakPtr     = std::weak_ptr<IPlayerTerminal>;

using IPrivilegedTerminal        = ITerminal<admin::Message>;
using IPrivilegedTerminalPtr     = std::shared_ptr<IPrivilegedTerminal>;
using IPrivilegedTerminalWeakPtr = std::weak_ptr<IPrivilegedTerminal>;


} // namespace network
