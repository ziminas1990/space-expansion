from typing import Optional, Any

from google.protobuf.message import Message
from .proxy_channel import ProxyChannel
from .channel import ChannelMode


class ProtobufChannel(ProxyChannel):
    """Implement protobuf channel, to exchange protobuf messages
     with remote side.
     Class does not implement network communication. It acts as proxy channel
     and implement encoding/decoding protobuf channel. It should be attached
     to another channel, that operates with 'bytes' messages (like UDP
     channel)"""

    def __init__(self, message_type: Any,
                 mode: ChannelMode=ChannelMode.PASSIVE,
                 channel_name: Optional[str] = None):
        super().__init__(mode=mode, proxy_name=channel_name)
        self._message_type = message_type

    # Override from Channel
    def encode(self, message: Message) -> bool:
        return message.SerializeToString()

    # overridden from ProxyChannel
    def decode(self, data: bytes) -> Optional[Message]:
        if not data:
            return None
        message = self._message_type()
        message.ParseFromString(data)
        return message