import logging.config
from typing import Optional

from expansion.transport import UdpChannel, ProtobufChannel, ProxyChannel
from expansion.modules.factory import module_factory

import expansion.api as api
from expansion.interfaces.rpc import AccessPanelI
from expansion.modules import Commutator

logging.basicConfig(level=logging.DEBUG)


async def login(server_ip: str,
                login_port: int,
                login: str,
                password: str)\
        -> (Optional[Commutator], Optional[str]):

    async def tunnel_factory() -> (Optional[ProxyChannel], Optional[str]):
        udp_channel = UdpChannel(
            on_closed_cb=lambda: logging.warning(f"Socket is closed!"))
        if not await udp_channel.open():
            return None, "Failed to open UDP connection!"
        udp_channel.set_remote(server_ip, login_port)

        protobuf_channel = ProtobufChannel(message_type=api.Message)
        access_panel = AccessPanelI()

        udp_channel.attach_to_terminal(protobuf_channel)
        protobuf_channel.attach_channel(udp_channel)
        access_panel.attach_to_channel(protobuf_channel)

        remote_port, error = await access_panel.login(login, password)
        if error is not None:
            return None, f"Login failed: {error}"
        udp_channel.set_remote(server_ip, remote_port)
        return protobuf_channel, None

    return Commutator(
        tunnel_factory=tunnel_factory,
        modules_factory=module_factory,
        name="Root"
    ), None
