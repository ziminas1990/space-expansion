from .channel import Channel
from .terminal import Terminal
from typing import Optional, Any
import abc

from expansion import utils


class ProxyChannel(Channel, Terminal):
    """Proxy channel is a channel, that can be placed between terminal
    and other channel. It may be used to implement messages converter
    or encoder/decoder.

    When subclassing, the 'decode()' and 'encode()' calls should be
    overridden.
    """

    def __init__(self,
                 proxy_name: Optional[str] = None,
                 trace_mode=False,
                 *args, **kwargs):
        self.proxy_name: str = proxy_name or utils.generate_name(ProxyChannel)
        super().__init__(channel_name=f"{self.proxy_name}/enc",
                         terminal_name=f"{self.proxy_name}/dec",
                         trace_mode=trace_mode,
                         *args, **kwargs)
        self.downlevel: Optional[Channel] = None
        self._trace_mode = trace_mode

    def get_name(self) -> str:
        return self.proxy_name

    def set_trace_mode(self, on: bool):
        self._trace_mode = on

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
    def attach_channel(self, channel: Channel):
        assert channel
        super().attach_channel(channel)  # For logging
        self.downlevel = channel

    # Override from Terminal
    def on_channel_detached(self):
        super().on_channel_detached()  # For logging
        self.downlevel = None

    # Override from Terminal
    def on_receive(self, message: Any, timestamp: Optional[int]):
        # For logging:
        super().on_receive(message, timestamp)
        decoded_message = self.decode(message)
        if decoded_message:
            self.on_message(decoded_message, timestamp)

    # Override from Channel
    def send(self, message: Any) -> bool:
        if self._trace_mode:
            self.channel_logger.debug(f"Sending:\n{message}")

        if not self.downlevel:
            self.channel_logger.warning(f"Not attached to the channel!")
            return False
        data = self.encode(message)
        if not data:
            self.channel_logger.warning(f"Failed to encode message!")
            return False
        return self.downlevel.send(data)

    # Override from Channel
    async def close(self):
        self.channel_logger.info("Closing channel...")
        if self.downlevel:
            await self.downlevel.close()
