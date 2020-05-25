from typing import Optional, Any, Dict, List, NamedTuple
import logging


import expansion.protocol.Protocol_pb2 as public
from expansion.protocol.utils import get_message_field
from expansion.transport.terminal import Terminal, BufferedTerminal
from expansion.transport.inversed_channel import InversedChannel
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


class ModuleInfo(NamedTuple):
    slot_id: int
    type: str
    name: str

    @staticmethod
    def from_protubuf(info: public.ICommutator.ModuleInfo) -> 'ModuleInfo':
        return ModuleInfo(slot_id=info.slot_id,
                          type=info.module_type,
                          name=info.module_name)


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

    async def get_module_info(self, slot_id: int) -> Optional[ModuleInfo]:
        """Return information about module, installed into the specified
        'slot_id'"""
        request = public.Message()
        request.commutator.module_info_req = slot_id
        self.send(request)
        response = await self.wait_message()
        if not response:
            return None
        module_info = get_message_field(response, ["commutator", "module_info"])
        if not module_info:
            return None
        return ModuleInfo.from_protubuf(module_info)

    async def get_all_modules(self) -> Optional[List[ModuleInfo]]:
        """Return all modules, attached to commutator"""
        success, total_slots = self.get_total_slots()
        if not success:
            return None
        request = public.Message()
        request.commutator.all_modules_info_req = True
        self.send(request)

        all_modules = []
        for i in range(total_slots):
            response = await self.wait_message()
            module_info = get_message_field(response, ["commutator", "module_info"])
            if not module_info:
                return None
            all_modules.append(ModuleInfo.from_protubuf(module_info))
        return all_modules

    async def open_tunnel(self, port: int, terminal: Terminal) -> Optional[str]:
        """Open tunnel to the specified 'port' and attach the specified
        'terminal' to it. Return None on success, or error string on fail."""
        request = public.Message()
        request.commutator.open_tunnel = port
        self.send(request)

        response = await self.wait_message()
        if not response:
            return None
        tunnel_id = get_message_field(response, ["commutator", "open_tunnel_report"])
        if not tunnel_id:
            error = get_message_field(response, ["commutator", "open_tunnel_failed"])
            if not error:
                error = "unexpected message received"
            self._logger.warning(f"Failed to open tunnel to port #{port}: error")
            return error

        tunnel: Tunnel = Tunnel(tunnel_id=tunnel_id,
                                client=terminal,
                                commutator=self)
        terminal.attach_channel(tunnel)

        self.tunnels.update({tunnel_id: tunnel})
        return None  # No errors

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


class RootCommutator(Commutator):

    def __init__(self):
        super().__init__(name="Root")
        self.inversed_channel: Optional[InversedChannel] = None

    def attach_channel(self, channel: InversedChannel):
        """Attach terminal to the specified 'channel'. This channel will be
        used to send messages"""
        assert isinstance(channel, InversedChannel),\
            "Root commutator may be attached only to the inversed channel"
        self.inversed_channel = channel
        super().attach_channel(channel)

    async def run(self):
        """Run commutator. Should be run in separate task"""
        await self.inversed_channel.run()

    async def stop(self):
        await self.inversed_channel.close()