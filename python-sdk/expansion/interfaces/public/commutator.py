from typing import Optional, Any, Dict, List, NamedTuple
import logging


import expansion.protocol.Protocol_pb2 as public
from expansion.protocol.utils import get_message_field
from expansion.transport.terminal import Terminal
from expansion.transport.proxy_channel import ProxyChannel
from expansion.transport.channel import Channel, ChannelMode
from .base_module import BaseModule


class Tunnel(ProxyChannel):
    """Tunnels are used to send and receive messages.
    From the client's (up level) point of view, tunnel is a channel, which may
    be used to send messages to the remote side and receive messages.
    Every message, sent by client, will be encapsulated to toplevel Message
    with some 'tunnel_id'.
    """

    def __init__(self, tunnel_id: int,
                 client: Terminal,
                 commutator: 'Commutator',
                 mode: ChannelMode = ChannelMode.PASSIVE,
                 channel_name: Optional[str] = None,
                 *args, ** kwargs):
        """Create tunnel for the specified 'client' with the specified
        'tunnel_id'. The specified 'channel' will be used by tunnel to send
        messages."""
        super().__init__(mode=mode, channel_name=channel_name, *args, **kwargs)
        self.tunnel_id: int = tunnel_id
        self.client: Terminal = client
        self.commutator: 'Commutator' = commutator

    # Overridden from ProxyChannel
    def decode(self, data: public.Message) -> Optional[Any]:
        """De-encapsulate message and pass it to client"""
        if data.tunnelId != self.tunnel_id:
            if self.logger:
                self.logger.warning(f"Tunnel_id mismatch! {data.tunnelId} != {self.tunnel_id}")
            return None
        return data.encapsulated

    # Overridden from ProxyChannel
    def encode(self, message: Any) -> Optional[Any]:
        """Encapsulate message in tunnel container"""
        container = public.Message()
        container.tunnelId = self.tunnel_id
        container.encapsulated.CopyFrom(message)
        return container

    # Overridden from Channel
    async def close(self):
        assert False, "Not implemented yet"


class ModuleInfo(NamedTuple):
    slot_id: int
    type: str
    name: str

    @staticmethod
    def from_protubuf(info: public.ICommutator.ModuleInfo) -> 'ModuleInfo':
        return ModuleInfo(slot_id=info.slot_id,
                          type=info.module_type,
                          name=info.module_name)


class Commutator(BaseModule):
    """Commutator is a:
    - BaseModule, because it represent a corresponding commutator module on
      the server;
    - Channel, because tunnels, uses commutator as channel to send messages.
    """

    def __init__(self, name: str = __name__):
        super().__init__(preferable_channel_mode=ChannelMode.ACTIVE)
        # Commutator prefers active channel to forward encapsulated messages
        # to the corresponding tunnels as soon as possible. See 'on_receive'
        # function for more details
        self.tunnels: Dict[int, Tunnel] = {}
        self._logger = logging.getLogger(f"{name}")

    # Override from Channel
    def send(self, message: Any) -> bool:
        """Send operation is implemented in BaseModule"""
        return self.send_message(message=message)

    # Override from BaseModule->Terminal
    def attach_channel(self, channel: Channel):
        if channel.mode != ChannelMode.ACTIVE:
            try:
                channel.set_mode(ChannelMode.ACTIVE)
            except AssertionError:
                assert False, "Commutator may work properly only with " \
                              " active downlevel channel"
        super().attach_channel(channel=channel)

    # Override from BaseModule->Terminal
    def on_channel_mode_changed(self, mode: 'ChannelMode'):
        # No need in further propagation
        assert mode == ChannelMode.ACTIVE,\
            "Commutator may work properly only with active downlevel channel!"

    # Override from BaseModule->Terminal
    async def close(self):
        assert False, "Operation is not supported for commutators"

    # Override from BaseModule->Terminal
    def on_receive(self, message: public.Message):
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

    async def get_total_slots(self) -> (bool, int):
        """Return total number of devices, attached to this commutator"""
        request = public.Message()
        request.commutator.total_slots_req = True
        self.send_message(request)
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
        self.send_message(request)
        response = await self.wait_message()
        if not response:
            return None
        module_info = get_message_field(response, ["commutator", "module_info"])
        if not module_info:
            return None
        return ModuleInfo.from_protubuf(module_info)

    async def get_all_modules(self) -> Optional[List[ModuleInfo]]:
        """Return all modules, attached to commutator"""
        success, total_slots = await self.get_total_slots()
        if not success:
            return None
        request = public.Message()
        request.commutator.all_modules_info_req = True
        self.send_message(request)

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
        self.send_message(request)

        response = await self.wait_message()
        if not response:
            return None
        tunnel_id = get_message_field(response, ["commutator", "open_tunnel_report"])
        if tunnel_id is None:
            error = get_message_field(response, ["commutator", "open_tunnel_failed"])
            if not error:
                error = "unexpected message received"
            self._logger.warning(f"Failed to open tunnel to port #{port}: {error}")
            return error

        tunnel: Tunnel = Tunnel(tunnel_id=tunnel_id,
                                client=terminal,
                                mode=ChannelMode.PASSIVE,
                                commutator=self)
        terminal.attach_channel(tunnel)
        # A channel is attached to the commutator, but tunnels are attached
        # to the channel directly
        tunnel.attach_channel(channel=self.channel)
        tunnel.attach_to_terminal(terminal)

        self.tunnels.update({tunnel_id: tunnel})
        return None  # No errors
