from typing import Any, Optional
import abc
import logging


class Terminal(abc.ABC):
    next_terminal_id: int = 0

    def __init__(self, terminal_name: str = "Transport.Terminal", *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.terminal_id = Terminal.next_terminal_id
        Terminal.next_terminal_id += 1
        self.terminal_name = f"{terminal_name}_{self.terminal_id}"
        self.terminal_logger = logging.getLogger(self.terminal_name)

    @abc.abstractmethod
    def on_receive(self, message: Any):
        """This callback will be called to pass received message, that was
        addressed to this terminal"""
        self.terminal_logger.debug(f"got {message}")

    @abc.abstractmethod
    def attach_channel(self, channel: 'Channel'):
        """Attach terminal to the specified 'channel'. This channel will be
        used to send messages"""
        self.terminal_logger.info(f"Attached to channel {channel.channel_name}")

    @abc.abstractmethod
    def on_channel_mode_changed(self, mode: 'ChannelMode'):
        """This function is called, when downlevel channel mode has been
        changed"""
        self.terminal_logger.info(f"Channel mode changed to {mode}")

    @abc.abstractmethod
    def on_channel_detached(self):
        """Detach terminal from channel. After it is done, terminal won't
        be able to send messages anymore"""
        self.terminal_logger.info(f"Channel detached")
