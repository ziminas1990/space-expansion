from typing import Optional, Any

from google.protobuf.message import Message

from .proxy_channel import ProxyChannel

from expansion import utils


class ProtobufChannel(ProxyChannel):
    """Implement protobuf channel, to exchange protobuf messages
     with remote side.
     Class does not implement network communication. It acts as proxy channel
     and implement encoding/decoding protobuf channel. It should be attached
     to another channel, that operates with 'bytes' messages (like UDP
     channel)"""

    def __init__(self, message_type: Any,
                 channel_name: Optional[str] = None,
                 trace_mode: bool = False):
        name = channel_name or utils.generate_name(ProxyChannel)
        super().__init__(proxy_name=name, trace_mode=trace_mode)
        self._message_type = message_type

    # Override from ProxyChannel
    def encode(self, message: Message) -> bool:
        return message.SerializeToString()

    # overridden from ProxyChannel
    def decode(self, data: bytes) -> Optional[Message]:
        if not data:
            return None
        message = self._message_type()
        message.ParseFromString(data)
        return message
