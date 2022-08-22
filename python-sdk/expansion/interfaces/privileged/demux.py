import logging
from typing import Optional, Dict

import expansion.api as api
from expansion.transport import Terminal, Channel


class Demux(Terminal, Channel):
    # Demux acting as a channel for a multiple terminals, and forwards
    # responses to a specific terminal, depending on response type

    def __init__(self, name: str, *args, **kwargs):
        super(Demux, self).__init__(
            terminal_name=f"{name}(Terminal)",
            channel_name=f"{name}(Channel)",
            *args, **kwargs)

        self.token: int = 0
        self.channel: Optional[Channel] = None
        self.terminals: Dict[str, Terminal] = {}

    def set_token(self, token: int):
        self.token = token

    def attach_terminals(self, terminals: Dict[str, Terminal]):
        self.terminals.update(terminals)

    def detach_terminals(self):
        self.terminals = {}

    def on_receive(self, message: api.admin.Message, timestamp: Optional[int]):
        try:
            encapsulated = message.WhichOneof("choice")
            terminal = self.terminals[encapsulated]
            if terminal:
                terminal.on_receive(message, message.timestamp)
        except KeyError:
            logging.error(f"Unexpected message received: {message}")
            return

    def attach_channel(self, channel: 'Channel'):
        self.channel = channel

    def on_channel_closed(self):
        self.channel = None

    def attach_to_terminal(self, terminal: 'Terminal'):
        assert False, "Operation makes no sense! Use attach_terminals()"

    def detach_terminal(self):
        assert False, "Operation makes no sense! Use detach_terminals()"

    def send(self, message: api.admin.Message) -> bool:
        # Acting as simple proxy
        message.token = self.token
        return self.channel and self.channel.send(message)

    async def close(self):
        if self.channel:
            await self.channel.close()
