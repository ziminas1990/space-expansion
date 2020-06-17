from enum import Enum
from typing import Any, Optional
import abc
import asyncio
import logging

from expansion import utils


class ChannelMode(Enum):
    PASSIVE = 2
    # Passive channel will never pass received data to the terminal. They will
    # save all received messages into internal queue and return them
    # 'on demand', through channel's 'receive()' call.
    ACTIVE = 1
    # Active channel will push all received data to terminal immediately
    # through terminal's 'on_receive()' call.


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
        is ready to be passe to client
     2. implement 'send()' function
     3. implement 'close()' function
     """

    next_channel_id: int = 0

    def __init__(self, mode: ChannelMode, channel_name=None, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.channel_name = channel_name or utils.generate_name(type(self))
        self.channel_logger = logging.getLogger(self.channel_name)

        self.mode: ChannelMode = mode
        self.queue: asyncio.Queue = asyncio.Queue()
        # Queue for all received messages
        self.terminal: Optional['Terminal'] = None

    def get_name(self) -> str:
        return self.channel_name

    def set_mode(self, mode: ChannelMode):
        """Set new mode for the channel"""
        if self.mode != mode:
            self.channel_logger.info(f"Changing mode: {self.mode} -> {mode}")
        self.mode: ChannelMode = mode
        if self.mode == ChannelMode.ACTIVE:
            # reset queue, cause we are not going to use it in active mode
            self.queue = asyncio.Queue()

    def propagate_mode(self, mode: ChannelMode):
        """The same as 'set_mode', but also change mode of uplevel
        terminal."""
        self.set_mode(mode)
        if self.terminal:
            self.terminal.on_channel_mode_changed(self.mode)

    def is_active_mode(self):
        return self.mode == ChannelMode.ACTIVE

    def on_message(self, message: Any):
        self.channel_logger.debug(f"Got:\n{message}")
        if self.is_active_mode():
            if self.terminal:
                self.terminal.on_receive(message)
        else:
            self.queue.put_nowait(message)

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

    async def receive(self, timeout: float = 5) -> Any:
        """Return message from queue or, if queue is empty, await for the
        next received message, but not more than 'timeout' seconds.
        If channel is in PASSIVE mode, return None immediately"""
        if self.is_active_mode():
            return None
        try:
            if self.queue.empty():
                while True:
                    return await asyncio.wait_for(self.queue.get(), timeout=timeout)
            else:
                return self.queue.get_nowait()
        except asyncio.TimeoutError:
            self.channel_logger.warning(f"Receive timeout occurred!")
            return None
