from typing import Any, Optional
import abc


class Terminal(abc.ABC):

    @abc.abstractmethod
    def on_receive(self, message: Any):
        """This callback will be called to pass received message, that was
        addressed to this terminal"""
        pass

    @abc.abstractmethod
    def attach_channel(self, channel: 'Channel'):
        """Attach terminal to the specified 'channel'. This channel will be
        used to send messages"""
        pass

    @abc.abstractmethod
    def on_channel_mode_changed(self, mode: 'ChannelMode'):
        """This function is called, when downlevel channel mode has been
        changed"""
        pass

    @abc.abstractmethod
    def on_channel_detached(self):
        """Detach terminal from channel. After it is done, terminal won't
        be able to send messages anymore"""
        pass
