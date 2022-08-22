from typing import Any, Optional, Tuple, List
import time
import asyncio

from expansion.api import get_message_field
from expansion.transport import Terminal, Channel, ChannelClosed


class IOTerminal(Channel, Terminal):
    """IOTerminal may be attached to the channel (as a terminal inheritor)
    and can be used by the client code as a channel. It uses internal queue
    to store all incoming messages until they are read by a client (see
    the 'wait_message' call).
    """

    def __init__(self,
                 name: Optional[str] = None,
                 trace_mode: bool = False,
                 *args, **kwargs):
        assert asyncio.get_running_loop() is not None
        super().__init__(channel_name=name, terminal_name=name, *args, **kwargs)
        self.queue: asyncio.Queue = asyncio.Queue()
        self._trace_mode = trace_mode

    def set_trace_mode(self, on: bool):
        self._trace_mode = on

    def get_name(self) -> str:
        return super(IOTerminal, self).get_name()

    def is_valid(self) -> bool:
        return self.channel is not None

    async def wait_message(self, timeout: float = 1.0) -> \
            Tuple[Optional[Any], Optional[int]]:
        """Await for a message on the internal queue for not more than the
        specified 'timeout' seconds. Return a message and an optional timestamp,
        when the message was sent."""
        try:
            message, timestamp = \
                await asyncio.wait_for(self.queue.get(), timeout=timeout)
            if get_message_field(message, ["session", "closed_ind"]):
                raise ChannelClosed()
            return message, timestamp
        except asyncio.TimeoutError:
            return None, None

    async def wait_exact(self, message: List[str], timeout: float = 1.0) \
            -> Tuple[Optional[Any], Optional[int]]:
        """Await for the specified 'message' but not more than 'timeout' seconds.
        Ignore all other received messages. Return expected message and timestamp or
        None"""
        while timeout > 0:
            start_at = time.monotonic()
            received_msg, timestemp = await self.wait_message(timeout)
            expected_msg = get_message_field(received_msg, message)
            if expected_msg is not None:
                return expected_msg, timestemp
            # Got unexpected message. Just ignoring it
            timeout -= time.monotonic() - start_at
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
        if self._trace_mode:
            self.terminal_logger.debug(f"Queue size: {self.queue.qsize()}")
