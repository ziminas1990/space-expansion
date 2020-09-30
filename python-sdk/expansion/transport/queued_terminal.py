from typing import Any, Optional
import asyncio
import threading

from .channel import Channel, ChannelMode
from .terminal import Terminal


class QueuedTerminal(Terminal):
    """Queued terminal make it easy to work with channels regardless of its
    mode. The 'wait_message' function can be used, to await messages from
    either ACTIVE or PASSIVE channels.
    If channel is in ACTIVE mode, then all received messages will be put
    into internal queue.
    This class implies inheritance. You can but should not create and use
    instance of this class directly
    The main restriction of this class - you can't handle unexpected messages
    (like some event indication). To do this, subclass Terminal instead
    """

    def __init__(self, terminal_name: Optional[str] = None, *args, **kwargs):
        super().__init__(terminal_name=terminal_name, *args, **kwargs)
        self.queue: asyncio.Queue = asyncio.Queue()
        self.channel: Optional[Channel] = None
        self.wait_lock: threading.Lock = threading.Lock()

    def get_name(self) -> str:
        return super(QueuedTerminal, self).get_name()

    # Override from Terminal
    def on_receive(self, message: Any):
        super().on_receive(message)  # For logging
        try:
            self.queue.put_nowait(message)
        except asyncio.QueueFull:
            self.terminal_logger.warning(f"Drop message: queue is full!")
        self.terminal_logger.debug(f"Queue size: {self.queue.qsize()}")

    # Override from Terminal
    def attach_channel(self, channel: Channel):
        super().attach_channel(channel)  # For logging
        self.channel = channel

    # Override from Terminal
    def on_channel_mode_changed(self, mode: ChannelMode):
        super().on_channel_mode_changed(mode)  # For logging

    # Override from Terminal
    def on_channel_detached(self):
        super().on_channel_detached()  # For logging
        self.channel = None

    async def wait_message(self, timeout: float = 1.0) -> Optional[Any]:
        # If attached channel is in PASSIVE mode, just recall its 'receive'
        # method. Otherwise, await for a message on the internal queue.
        assert not self.wait_lock.locked()  # Multithreading wait is not what you want!
        with self.wait_lock:
            if self.channel and not self.channel.is_active_mode():
                return await self.channel.receive(timeout)
            try:
                return await asyncio.wait_for(self.queue.get(), timeout=timeout)
            except asyncio.TimeoutError:
                return None

    def send_message(self, message: Any) -> bool:
        return self.channel is not None and self.channel.send(message=message)