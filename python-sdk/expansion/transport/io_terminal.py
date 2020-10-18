from typing import Any, Optional, Tuple
import asyncio

from .channel import Channel
from .terminal import Terminal


class IOTerminal(Channel, Terminal):
    """IOTerminal may be attached to the channel (as a terminal inheritor)
    and can be used by the client code as a channel. It uses internal queue
    to store all incoming messages until they are read by a client (see
    the 'wait_message' call).
    """

    def __init__(self, channel_name: Optional[str] = None, *args, **kwargs):
        super().__init__(channel_name=channel_name, *args, **kwargs)
        self.queue: asyncio.Queue = asyncio.Queue()
        self.channel: Optional[Channel] = None
        self._waiters: int = 0

    def get_name(self) -> str:
        return super(IOTerminal, self).get_name()

    async def wait_message(self, timeout: float = 1.0) -> Tuple[Optional[Any], Optional[int]]:
        """Await for a message on the internal queue for not more than the
        specified 'timeout' seconds. Return a message and a optional timestamp, when
        the message was sent."""
        try:
            return await asyncio.wait_for(self.queue.get(), timeout=timeout)
        except asyncio.TimeoutError:
            return None, None

    def wrap_channel(self, down_level: Channel):
        self.attach_channel(down_level)
        down_level.attach_to_terminal(self)

    # Override from Channel
    def attach_to_terminal(self, terminal: 'Terminal'):
        assert False, "Attaching IOTerminal to the terminal makes no sense"

    # Override from Channel
    def detach_terminal(self):
        assert False, "Detaching IOTerminal from the terminal makes no sense"

    # Override from Channel
    def send(self, message: Any) -> bool:
        return self.channel is not None and self.channel.send(message)

    # Override from Channel
    async def close(self):
        """Close channel"""
        if self.channel is not None:
            await self.channel.close()

    # Override from Terminal
    def on_receive(self, message: Any, timestamp: Optional[int]):
        super().on_receive(message, timestamp)  # For logging
        try:
            self.queue.put_nowait((message, timestamp))
        except asyncio.QueueFull:
            self.terminal_logger.warning(f"Drop message: queue is full!")
        self.terminal_logger.debug(f"Queue size: {self.queue.qsize()}")

    # Override from Terminal
    def attach_channel(self, channel: Channel):
        super().attach_channel(channel)  # For logging
        self.channel = channel

    # Override from Terminal
    def on_channel_detached(self):
        super().on_channel_detached()  # For logging
        self.channel = None
