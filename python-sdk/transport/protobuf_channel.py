
from typing import Optional, Any
import logging

from google.protobuf.message import Message
from .channel import Channel


class ProtobufChannel(Channel):
    """Implement protobuf channel, to exchange protobuf messages
     with remote side.
    Note that this class does NOT implement network communication! It
    uses another channel as downlevel to send and receive bytes messages.
    You should call the 'attach_to_channel()' method, before perform any
    'send()' or 'receive()' calls. The attached channel should operate with
    'bytes' messages. You may use the 'UdpChannel' for example"""

    def __init__(self, name: str, toplevel_message_type: Any):
        """Create protobuf channel. It uses downlevel channel to send/receive
        bytes messages (see the 'attach_to_channel()' method). The specified
        'toplevel_message_type' class will be instantiated as top level
        message, when decoding received message. The specified 'name' will
        be used to log messages."""
        self._downlevel: Optional[Channel] = None
        self._name = name
        self._toplevel_message_type = toplevel_message_type
        self._logger = logging.getLogger(f"{__name__} {self._name}")

    def attach_to_channel(self, channel: Channel):
        """Attach channel to the specified 'channel' as downlevel. Specified
        channel should operate with 'bytes' messages"""
        self._downlevel = channel

    def send(self, message: Message):
        """Write the specified 'message' to channel"""
        self._logger.debug(f"Sending:\n{message}")
        self._downlevel.send(message.SerializeToString())

    async def receive(self, timeout: float = 5) -> Optional[Message]:
        """Await for the message, but not more than 'timeout' seconds"""
        data: bytes = await self._downlevel.receive(timeout)
        if not data:
            return None
        message = self._toplevel_message_type()
        message.ParseFromString(data)
        self._logger.debug(f"Got:\n{message}")
        return message
