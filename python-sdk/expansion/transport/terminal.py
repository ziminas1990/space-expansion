from typing import Any, Optional
import abc
import asyncio
from asyncio.queues import Queue

from .channel import Channel


class Terminal(abc.ABC):

    def __init__(self):
        self.channel: Optional[Channel] = None
        # Channel, that will be used to send messages

    @abc.abstractmethod
    def on_receive(self, message: Any):
        """This callback will be called to pass received message, that was
        addressed to this terminal"""
        pass

    def send(self, message: Any) -> bool:
        """Send the specified 'message' to the remote side. Return 'True' on
        success, otherwise return False."""
        return self.channel and self.channel.send(message)

    def attach_channel(self, channel: Channel):
        """Attach terminal to the specified 'channel'. This channel will be
        used to send messages"""
        self.channel = channel

    def on_channel_detached(self):
        """Detach terminal from channel. After it is done, terminal won't
        be able to send messages anymore"""
        self.channel = None


class BufferedTerminal(Terminal):

    def __init__(self):
        super().__init__()
        self.queue: Queue = Queue()

    def on_receive(self, message: Any):
        self.queue.put_nowait(message)

    async def wait_message(self, timeout: float = 1.0) -> Optional[Any]:
        try:
            return await asyncio.wait_for(self.queue.get(), timeout=timeout)
        except asyncio.TimeoutError:
            return None
