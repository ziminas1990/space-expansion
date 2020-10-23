from enum import Enum
from typing import Any, Optional
import abc
import logging

from expansion import utils


class Channel(abc.ABC):
    """Channel is abstraction, that provide interface to exchange with some
    messages. Messages may have any type, depending on channel implementation.
    Channel may work in two modes: passive and active.
    In passive mode channel won't pass any received messages to the client
    (terminal). Messages will be stored in internal queue instead, and may be
    given to client through 'receive()' call. If there is no messages in
    queue, then 'receive()' call will block until message is received, or
    timeout occurred.
    In active mode channel will pass all received data to the client. The
    'receive()' call will always return None immediately. If terminal is not
     attached, then all received will be ignored (dropped).

     Subclassing:
     1. the 'on_message' function should be called for each message, that
        is ready to be passed to the client
     2. implement 'send()' function
     3. implement 'close()' function
     """

    next_channel_id: int = 0

    def __init__(self, channel_name=None,
                 trace_mode=False,
                 *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.channel_name = channel_name or utils.generate_name(type(self))
        self.channel_logger = logging.getLogger(self.channel_name)
        self.terminal: Optional['Terminal'] = None
        self._trace_mode = trace_mode

    def get_name(self) -> str:
        return self.channel_name

    def set_trace_mode(self, on: bool):
        self._trace_mode = on

    def on_message(self, message: Any, timestamp: Optional[int]):
        if self._trace_mode:
            self.channel_logger.debug(f"Got:\n{message}, timestamp = {timestamp}")
        if self.terminal:
            self.terminal.on_receive(message, timestamp)

    def attach_to_terminal(self, terminal: 'Terminal'):
        """Attach channel to terminal. If channel in ACTIVE mode, it will pass
        all messages to attached terminal. Otherwise, attaching terminal has
        no sense."""
        self.terminal = terminal
        self.channel_logger.info(f"Attached to terminal {self.terminal.terminal_name}")

    def detach_terminal(self):
        """Detach channel from terminal.
        Note that if channel is in ACTIVE mode, but is not attached to
        terminal, than all received messages will be dropped."""
        self.terminal = None
        self.channel_logger.info(f"Terminal detached")

    @abc.abstractmethod
    def send(self, message: Any) -> bool:
        """Write the specified 'message' to channel. Return true on success,
        otherwise return false"""
        pass

    @abc.abstractmethod
    async def close(self):
        """Close channel"""
        pass
