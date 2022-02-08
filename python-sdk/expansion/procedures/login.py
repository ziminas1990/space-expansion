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
                password: str,
                local_ip: str)\
        -> (Optional[Commutator], Optional[str]):

    # UDP channel, that will be used to login:
    login_udp_channel = UdpChannel(
        on_closed_cb=lambda: logging.error(f"Logging connection was closed"),
        channel_name="Login UDP")
    if not await login_udp_channel.open(server_ip, login_port):
        return None, "Failed to open login UDP connection!"

    # Protobuf channel for logging in
    login_channel = ProtobufChannel(channel_name="Login",
                                    message_type=api.Message)
    login_channel.attach_channel(login_udp_channel)
    login_udp_channel.attach_to_terminal(login_channel)

    # Creating access panel instance
    access_panel = AccessPanelI()
    access_panel.attach_to_channel(login_channel)

    async def tunnel_factory() -> (Optional[ProxyChannel], Optional[str]):
        player_udp_channel = UdpChannel(
            on_closed_cb=lambda: logging.warning(
                f"Socket is closed!"),
            channel_name="UDP.{port}")
        if not await player_udp_channel.bind():
            return None, "Can't bind UDP socket"
        local_port = player_udp_channel.local_addr[1]

        player_port, error = await access_panel.login(
            login=login,
            password=password,
            local_ip=local_ip,
            local_port=local_port)
        if error is not None:
            return None, "Login failed!"
        player_udp_channel.set_remote(server_ip, player_port)

        # Create protobuf layer
        tunnel = ProtobufChannel(
            channel_name=f"Root.{local_port}",
            message_type=api.Message)
        tunnel.attach_channel(player_udp_channel)
        player_udp_channel.attach_to_terminal(tunnel)
        return tunnel, None

    return Commutator(
        tunnel_factory=tunnel_factory,
        modules_factory=module_factory,
        name="Root"
    )
