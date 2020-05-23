from typing import Optional, Any, Dict, List
import logging


import expansion.protocol.Protocol_pb2 as public
from expansion.protocol.utils import get_message_field
from expansion.transport.terminal import Terminal, BufferedTerminal
from expansion.transport.channel import Channel


class Tunnel(Channel):
    """Tunnels are used to send and receive messages.
    From the client's (up level) point of view, tunnel is a channel, which may
    be used to send messages to the remote side and receive messages.
    Every message, sent by client, will be encapsulated to toplevel Message
    with some 'tunnel_id'.
    """

    def __init__(self, tunnel_id: int,
                 client: Terminal,
                 commutator: 'Commutator'):
        """Create tunnel for the specified 'client' with the specified
        'tunnel_id'. The specified 'channel' will be used by tunnel to send
        messages."""
        self.tunnel_id: int = tunnel_id
        self.client: Terminal = client
        self.commutator: 'Commutator' = commutator

    def send(self, message: Any) -> bool:
        """Encapsulate message in tunnel container and send it"""
        container = public.Message()
        container.tunnelId = self.tunnel_id
        container.encapsulated = message
        return self.commutator.send(message)

    async def receive(self, timeout: float = 5) -> Any:
        assert False, "Tunnel can't be used to get messages on demand!"

    def on_receive(self, message: Any):
        """De-encapsulate message and pass it to client"""
        self.client.on_receive(message.encapsulated)


class Commutator(BufferedTerminal):

    def __init__(self, name: str = __name__):
        super().__init__()
        self.tunnels: Dict[int, Tunnel] = {}
        self._logger = logging.getLogger(f"{name}")

    async def get_total_slots(self) -> (bool, int):
        """Return total number of devices, attached to this commutator"""
        request = public.Message()
        request.commutator.total_slots_req = True
        self.send(request)
        response = await self.wait_message()
        if not response:
            return False, 0
        total_slots = get_message_field(response, ["commutator", "total_slots"])
        return total_slots is not None, total_slots

    def on_receive(self, message: Any):
        if message.WhichOneof('choice') == 'encapsulated':
            try:
                tunnel = self.tunnels[message.tunnelId]
                if tunnel:
                    tunnel.on_receive(message)
                else:
                    self._logger.warning(
                        f"Ignore message for invalid tunnel"
                        f" #{message.tunnelId}:\n {message}")
            except KeyError:
                self._logger.warning(
                    f"Ignore message for invalid tunnel"
                    f" #{message.tunnelId}:\n {message}")
                return
        else:
            super().on_receive(message)

