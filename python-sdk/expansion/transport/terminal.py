from typing import Any, Optional, TYPE_CHECKING
import abc
import logging

from expansion import utils

if TYPE_CHECKING:
    from expansion.transport import Channel


class Terminal(abc.ABC):
    """Terminal is an entity that handles incoming messages.
    Terminal should be attached to channel so that it can send messages over
    channel."""

    def __init__(self,
                 terminal_name: str = None,
                 trace_mode=False,
                 *args, **kwargs):
        super(Terminal, self).__init__(*args, **kwargs)
        self.terminal_name = terminal_name or utils.generate_name(type(self))
        self.terminal_logger = logging.getLogger(self.terminal_name)
        self.channel: Optional["Channel"] = None
        self._trace_mode = trace_mode

    def get_name(self) -> str:
        return self.terminal_name

    def set_trace_mode(self, on: bool):
        self._trace_mode = on

    def trace_mode(self) -> bool:
        return self._trace_mode

    @abc.abstractmethod
    def on_receive(self, message: Any, timestamp: Optional[int]):
        """This callback will be called to pass received message, that was
        addressed to this terminal"""
        if self._trace_mode:
            self.terminal_logger.debug(f"Got\n{message}")

    def attach_channel(self, channel: 'Channel'):
        """Attach terminal to the specified 'channel'. This channel will be
        used to send messages"""
        self.terminal_logger.info(f"Attached to channel {channel.channel_name}")
        self.channel = channel

    def detach_channel(self):
        self.terminal_logger.info(f"Channel has been detached")
        self.channel = None

    def on_channel_closed(self):
        """Detach terminal from channel. After it is done, terminal won't
        be able to send messages anymore"""
        self.terminal_logger.info(f"Channel was closed")
        self.channel = None
