from typing import Optional, Any

from .terminal import Terminal
from .channel import Channel


class InversedChannel(Channel):
    """The inversed channel doesn't provide 'receive()' function, but
    reading channel and deliver all received message to terminal."""

    def __init__(self, donwlevel: Channel):
        self.donwlevel = donwlevel
        self.terminal: Optional[Terminal] = None
        self.closed = False

    def attach_to_terminal(self, terminal: Terminal):
        self.terminal = terminal

    def send(self, message: Any) -> bool:
        return self.donwlevel.send(message)

    async def run(self):
        while not self.closed:
            message = await self.donwlevel.receive()
            if self.terminal:
                self.terminal.on_receive(message)

    async def receive(self, timeout: float = 5) -> Any:
        assert False, "Inversed channel doesn't support 'receive()' call!"

    async def close(self):
        self.closed = True
        await self.donwlevel.close()