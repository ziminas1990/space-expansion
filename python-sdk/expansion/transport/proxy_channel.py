from .channel import Channel, ChannelMode
from .terminal import Terminal
from typing import Optional, Any
import logging
import abc


class ProxyChannel(Channel, Terminal):
    """Proxy channel is a channel, that can be placed between terminal
    and other channel. It may be used to implement messages converter
    or encoder/decoder.

    When subclassing, the 'decode()' and 'encode()' calls should be
    overridden.
    """

    def __init__(self, mode: ChannelMode,
                 channel_name: Optional[str] = None,
                 *args, **kwargs):
        super().__init__(mode=mode, *args, **kwargs)
        self.logger = logging.getLogger(f"{__name__} '{channel_name}'") \
            if channel_name else None
        self.downlevel: Optional[Channel] = None

    @abc.abstractmethod
    def decode(self, data: Any) -> Optional[Any]:
        """Decode the specified 'data', received from down-level channel
        to the message, that may be passed to client (terminal).  Return None
        if message has not been converted."""
        pass

    @abc.abstractmethod
    def encode(self, message: Any) -> Optional[Any]:
        """Encode the specified 'message', received from client (terminal)
        to the data, that may be passed to down-level channel.  Return None
        if message has not been encoded."""
        pass

    # Override from Terminal
    def attach_channel(self, channel: 'Channel'):
        assert channel
        self.downlevel = channel
        assert channel.mode == self.mode,\
            "Downlevel must be in the same mode as proxy channel"

    # Override from Terminal
    def on_channel_mode_changed(self, mode: 'ChannelMode'):
        """Downlevel channel has changed mode, so proxy channel should
        also change its mode and propagate mode to uplevel terminal"""
        self.propagate_mode(mode)

    # Override from Terminal
    def on_channel_detached(self):
        self.downlevel = None

    # Override from Terminal
    def on_receive(self, message: Any):
        decoded_message = self.decode(message)
        if decoded_message:
            if self.logger:
                self.logger.debug(f"Received:\n{decoded_message}")
            self.on_message(decoded_message)

    # Override from Channel
    def set_mode(self, mode: ChannelMode):
        """Set new mode for the channel"""
        if self.downlevel:
            if mode == ChannelMode.ACTIVE:
                assert self.downlevel.mode == ChannelMode.ACTIVE,\
                    "Can't switch proxy channel to Active mode, while"\
                    " downlevel channel is not in Active mode"
        super().set_mode(mode=mode)

    # Override from Channel
    def send(self, message: Any) -> bool:
        if not self.downlevel:
            return False
        data = self.encode(message)
        if not data:
            return False
        if self.logger:
            self.logger.debug(f"Send:\n{message}")
        return self.downlevel.send(data)

    # Override from Channel
    async def receive(self, timeout: float = 5) -> Any:
        if not self.downlevel or self.is_active_mode():
            return None
        data = await self.downlevel.receive(timeout)
        if not data:
            return None
        message = self.decode(data=data)
        if message and self.logger:
            self.logger.debug(f"Received:\n{message}")
        return message

    # Override from Channel
    async def close(self):
        if self.downlevel:
            await self.downlevel.close()