from typing import Optional, Any, Dict, List, NamedTuple
import logging


import expansion.protocol.Protocol_pb2 as public
from expansion.protocol.utils import get_message_field
from expansion.transport.terminal import Terminal
from expansion.transport.queued_terminal import QueuedTerminal
from expansion.transport.proxy_channel import ProxyChannel
from expansion.transport.channel import Channel


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
                 *args, ** kwargs):
        """Create tunnel for the specified 'client' with the specified
        'tunnel_id'. The specified 'channel' will be used by tunnel to send
        messages."""
        tunnel_name = f"{commutator.get_name()}::tun#{tunnel_id}"
        super().__init__(proxy_name=tunnel_name, *args, **kwargs)
        self.tunnel_id: int = tunnel_id
        self.client: Terminal = client
        self.commutator: 'Commutator' = commutator

    # Overridden from ProxyChannel
    def decode(self, data: public.Message) -> Optional[Any]:
        """De-encapsulate message and pass it to client"""
        if data.tunnelId != self.tunnel_id:
            self.terminal_logger.warning(f"Tunnel_id mismatch! {data.tunnelId} != {self.tunnel_id}")
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


class Commutator(QueuedTerminal):
    """Commutator is a:
    - BaseModule, because it represent a corresponding commutator module on
      the server;
    - Channel, because tunnels, uses commutator as channel to send messages.
    """

    def __init__(self, name: str = __name__):
        super().__init__(module_name=name)
        # Commutator prefers active channel to forward encapsulated messages
        # to the corresponding tunnels as soon as possible. See 'on_receive'
        # function for more details
        self.tunnels: Dict[int, Tunnel] = {}
        # All opened tunnels
        self.modules_cache: Dict[int, ModuleInfo] = {}
        # Key - slot_id, value - ModuleInfo
        self._logger = logging.getLogger(f"{name}")

    # Override from Channel
    def send(self, message: Any) -> bool:
        """Send operation is implemented in BaseModule"""
        return self.send_message(message=message)

    # Override from BaseModule->Terminal
    def attach_channel(self, channel: Channel):
        super().attach_channel(channel=channel)

    # Override from QueuedTerminal->Terminal
    async def close(self):
        assert False, "Operation is not supported for commutators"

    # Override from QueuedTerminal->Terminal
    def on_receive(self, message: public.Message):
        if message.WhichOneof('choice') == 'encapsulated':
            try:
                tunnel = self.tunnels[message.tunnelId]
            except KeyError:
                self._logger.warning(
                    f"Ignore message for invalid tunnel"
                    f" #{message.tunnelId}:\n {message}")
                return

            if tunnel:
                tunnel.on_receive(message)
            else:
                self._logger.warning(
                    f"Ignore message for invalid tunnel"
                    f" #{message.tunnelId}:\n {message}")
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
        total_slots = get_message_field(response, "commutator.total_slots")
        return total_slots is not None, total_slots

    async def get_module_info(self, slot_id: int,  use_cache: bool = True)\
            -> Optional[ModuleInfo]:
        """Return information about module, installed into the specified
        'slot_id'. If an internal cache is initialized, than data will be
        retrieved from it, unless 'use_cache' is False.
        """
        if use_cache and slot_id in self.modules_cache:
            return self.modules_cache[slot_id]
        request = public.Message()
        request.commutator.module_info_req = slot_id
        self.send_message(request)
        response = await self.wait_message()
        if not response:
            return None
        module_info = get_message_field(response, "commutator.module_info")
        if not module_info:
            return None
        info = ModuleInfo.from_protubuf(module_info)
        self.modules_cache.update({slot_id: info})
        return info

    async def get_all_modules(self, reset_cache: bool = False)\
            -> Optional[List[ModuleInfo]]:
        """Return all modules, attached to commutator. Modules received will
        be stored to a local cache. If the specified 'reset_cache' flag is
        true, then the cache will be cleared and a request will be sent to
        the server anyway"""
        if reset_cache:
            self.modules_cache.clear()
        if self.modules_cache:
            return list(self.modules_cache.values())

        success, total_slots = await self.get_total_slots()
        if not success:
            return None
        request = public.Message()
        request.commutator.all_modules_info_req = True
        self.send_message(request)

        for i in range(total_slots):
            response = await self.wait_message()
            module_info = get_message_field(response, "commutator.module_info")
            if not module_info:
                return None
            info = ModuleInfo.from_protubuf(module_info)
            self.modules_cache.update({info.slot_id: info})
        return list(self.modules_cache.values())

    async def open_tunnel(self, port: int, terminal: Terminal) -> Optional[str]:
        """Open tunnel to the specified 'port' and attach the specified
        'terminal' to it. Return None on success, or error string on fail."""
        request = public.Message()
        request.commutator.open_tunnel = port
        self.send_message(request)

        response = await self.wait_message()
        if not response:
            return None
        tunnel_id = get_message_field(response, "commutator.open_tunnel_report")
        if tunnel_id is None:
            error = get_message_field(response, "commutator.open_tunnel_failed")
            if not error:
                error = "unexpected message received"
            self._logger.warning(f"Failed to open tunnel to port #{port}: {error}")
            return error

        tunnel: Tunnel = Tunnel(tunnel_id=tunnel_id,
                                client=terminal,
                                commutator=self)
        terminal.attach_channel(tunnel)
        # A channel is attached to the commutator, but tunnels are attached
        # to the channel directly
        tunnel.attach_channel(channel=self.channel)
        tunnel.attach_to_terminal(terminal)

        self.tunnels.update({tunnel_id: tunnel})
        return None  # No errors
