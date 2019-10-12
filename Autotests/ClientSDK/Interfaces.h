#pragma once

#include <memory>
#include <Protocol.pb.h>

namespace autotests { namespace client {

class IProceedable
{
public:
  virtual ~IProceedable() = default;

  virtual void proceed(uint32_t nDeltaUs) = 0;
  virtual bool isComplete() const = 0;
};

class IClientChannel
{
public:
  virtual ~IClientChannel() = default;

  virtual bool send(spex::Message const& message) = 0;
};

class IClientTerminal
{
public:
  virtual ~IClientTerminal() = default;

  virtual void onMessageReceived(spex::Message&& message) = 0;
};

using IClientChannelPtr  = std::shared_ptr<IClientChannel>;
using IClientTerminalPtr = std::shared_ptr<IClientTerminal>;

using IClientTerminalWeakPtr = std::weak_ptr<IClientTerminal>;
using IClientChannelWeakPtr  = std::weak_ptr<IClientChannel>;

}}  // namespace autotests::client
