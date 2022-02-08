from typing import Optional, Any, Dict, List, NamedTuple, Tuple
import logging
from enum import Enum

import expansion.api as api
from expansion.api.utils import get_message_field
from expansion.transport import IOTerminal
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
                 commutator: 'CommutatorI',
                 *args, ** kwargs):
        """Create tunnel for the specified 'client' with the specified
        'tunnel_id'. The specified 'channel' will be used by tunnel to send
        messages."""
        tunnel_name = f"{commutator.get_name()}::tun#{tunnel_id}"
        super().__init__(proxy_name=tunnel_name, *args, **kwargs)
        self.tunnel_id: int = tunnel_id
        self.commutator: 'CommutatorI' = commutator

    # Overridden from ProxyChannel
    def decode(self, data: api.Message) -> Optional[Any]:
        """De-encapsulate message"""
        if data.tunnelId != self.tunnel_id:
            self.terminal_logger.warning(f"Tunnel_id mismatch! {data.tunnelId} != {self.tunnel_id}")
            return None
        return data.encapsulated

    # Overridden from ProxyChannel
    def encode(self, message: Any) -> Optional[Any]:
        """Encapsulate message in tunnel container"""
        container = api.Message()
        container.tunnelId = self.tunnel_id
        container.encapsulated.CopyFrom(message)
        return container

    # Overridden from Channel
    async def close(self):
        return await self.commutator.close_tunnel(self.tunnel_id)


class ModuleInfo(NamedTuple):
    slot_id: int
    type: str
    name: str

    @staticmethod
    def from_protubuf(info: api.ICommutator.ModuleInfo) -> 'ModuleInfo':
        return ModuleInfo(slot_id=info.slot_id,
                          type=info.module_type,
                          name=info.module_name)


class CommutatorI(IOTerminal):
    """Commutator is a module, that can be used to access another modules,
    attached to the commutator
    """

    class Status(Enum):
        SUCCESS = "success"
        INVALID_SLOT = "invalid slot"
        MODULE_OFFLINE = "module offline"
        REJECTED_BY_MODULE = "rejected by module"
        INVALID_TUNNEL = "invalid tunnel"
        # Internal SDK statuses:
        FAILED_TO_SEND_REQUEST = "failed to send request"
        RESPONSE_TIMEOUT = "response timeout"
        UNEXPECTED_RESPONSE = "unexpected response"
        CHANNEL_CLOSED = "channel closed"

        def is_success(self):
            return self == CommutatorI.Status.SUCCESS

        @staticmethod
        def convert(status: api.ICommutator.Status) -> "CommutatorI.Status":
            ProtobufStatus = api.ICommutator.Status
            ModuleStatus = CommutatorI.Status
            return {
                ProtobufStatus.SUCCESS: ModuleStatus.SUCCESS,
                ProtobufStatus.INVALID_SLOT: ModuleStatus.INVALID_SLOT,
                ProtobufStatus.MODULE_OFFLINE: ModuleStatus.MODULE_OFFLINE,
                ProtobufStatus.REJECTED_BY_MODULE: ModuleStatus.REJECTED_BY_MODULE,
                ProtobufStatus.INVALID_TUNNEL: ModuleStatus.INVALID_TUNNEL,
            }[status]

    def __init__(self, name: str = __name__):
        super().__init__(name=name)
        # Commutator prefers active channel to forward encapsulated messages
        # to the corresponding tunnels as soon as possible. See 'on_receive'
        # function for more details
        self.tunnels: Dict[int, Tunnel] = {}
        # All opened tunnels
        self._logger = logging.getLogger(f"{name}")

    # Override from BaseModule->Terminal
    def attach_channel(self, channel: Channel):
        super().attach_channel(channel=channel)

    # Override from IOTerminal->Terminal
    async def close(self):
        assert False, "Operation is not supported for commutators"

    # Override from IOTerminal->Terminal
    def on_receive(self, message: api.Message, timestamp: Optional[int]):
        if message.timestamp is not None:
            timestamp = message.timestamp

        if message.WhichOneof('choice') == 'encapsulated':
            if message.encapsulated.timestamp is not None:
                timestamp = message.encapsulated.timestamp

            try:
                tunnel = self.tunnels[message.tunnelId]
            except KeyError:
                self._logger.warning(
                    f"Ignore message for invalid tunnel"
                    f" #{message.tunnelId}:\n {message}")
                return

            if tunnel:
                # Encapsulated timestamp has a higher priority then a timestamp
                # passed to this function
                tunnel.on_receive(message, timestamp)
            else:
                self._logger.warning(
                    f"Ignore message for invalid tunnel"
                    f" #{message.tunnelId}:\n {message}")
        else:
            super().on_receive(message, timestamp)

    @Channel.return_on_close(False, 0)
    async def get_total_slots(self) -> (bool, int):
        """Return total number of devices, attached to this commutator"""
        request = api.Message()
        request.commutator.total_slots_req = True
        self.send(request)
        response, _ = await self.wait_message()
        if not response:
            return False, 0
        total_slots = get_message_field(response, ["commutator", "total_slots"])
        return total_slots is not None, total_slots

    @Channel.return_on_close(None)
    async def get_module_info(self, slot_id: int)\
            -> Optional[ModuleInfo]:
        """Return information about module, installed into the specified
        'slot_id'.
        """
        request = api.Message()
        request.commutator.module_info_req = slot_id
        self.send(request)
        response, _ = await self.wait_message()
        if not response:
            return None
        module_info = get_message_field(response, ["commutator", "module_info"])
        if not module_info:
            return None
        info = ModuleInfo.from_protubuf(module_info)
        return info

    @Channel.return_on_close(None)
    async def get_all_modules(self) -> Optional[List[ModuleInfo]]:
        """Return all modules, attached to commutator. Modules received will
        be stored to a local cache"""
        success, total_slots = await self.get_total_slots()
        if not success:
            return None
        request = api.Message()
        request.commutator.all_modules_info_req = True
        self.send(request)

        result: List[ModuleInfo] = []
        for i in range(total_slots):
            response, _ = await self.wait_message()
            module_info = get_message_field(response, ["commutator", "module_info"])
            if not module_info:
                return None
            result.append(ModuleInfo.from_protubuf(module_info))
        return result

    @Channel.return_on_close(Status.CHANNEL_CLOSED, None)
    async def open_tunnel(self, port: int) -> Tuple[Status, Optional[Tunnel]]:
        """Open tunnel to the specified 'port'. Return (tunnel, None) on
        success, otherwise return (None, error)"""
        request = api.Message()
        request.commutator.open_tunnel = port
        self.send(request)

        response, _ = await self.wait_message(timeout=0.1)  # it shouldn't take much time
        if not response:
            return CommutatorI.Status.RESPONSE_TIMEOUT, None
        tunnel_id = get_message_field(response, ["commutator", "open_tunnel_report"])
        if tunnel_id is None:
            error = get_message_field(response, ["commutator", "open_tunnel_failed"])
            if not error:
                return CommutatorI.Status.UNEXPECTED_RESPONSE, None
            self._logger.warning(f"Failed to open tunnel to port #{port}: {error}")
            return CommutatorI.Status.convert(error), None

        tunnel = Tunnel(tunnel_id=tunnel_id, commutator=self)
        tunnel.attach_channel(channel=self.channel)
        self.tunnels.update({tunnel.tunnel_id: tunnel})
        return CommutatorI.Status.SUCCESS, tunnel

    @Channel.return_on_close(Status.CHANNEL_CLOSED)
    async def close_tunnel(self, tunnel_id: int) -> Status:
        request = api.Message()
        request.commutator.close_tunnel = tunnel_id
        self.send(request)

        response, _ = await self.wait_message(timeout=0.2)  # it shouldn't take much time
        if not response:
            return CommutatorI.Status.RESPONSE_TIMEOUT

        status = get_message_field(response, ["commutator", "close_tunnel_status"])
        if status is None:
            return CommutatorI.Status.UNEXPECTED_RESPONSE
        return CommutatorI.Status.convert(status)
