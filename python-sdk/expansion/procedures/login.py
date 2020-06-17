import asyncio
import logging.config
from typing import Optional

from expansion.transport.channel import ChannelMode
from expansion.transport.udp_channel import UdpChannel
from expansion.transport.protobuf_channel import ProtobufChannel

import expansion.protocol.Protocol_pb2 as public
from expansion.interfaces.public.IAccessPanel import IAccessPanel
from expansion.interfaces.public.commutator import Commutator

logging.basicConfig(level=logging.DEBUG)


async def login(server_ip: str, login_port: int,
                login: str, password: str,
                local_ip: str, local_port: int) -> (Optional[Commutator], str):
    """Login on server and return root commutator.
    On success return (Commutator, None) pair. Otherwise return (None, error).
    This procedure will send login request to the specified 'server_ip' and
    'login_port' with the specified 'login' and 'password'. The specified
    'local_ip' and 'local_port' will be used to bind local UDP socket.
    Example:
        commutator, error = login("192.168.0.15", 6842, "Rotshild", "Money",
                                  "192.168.0.10", 4742)
        if error:
            self._logger.error(f"Failed to login: {error}"
            exit(1)
        asyncio.create_task(inversed_channel.run())

    """
    # UDP channel, that will be used to login:
    login_udp_channel = UdpChannel(on_closed_cb=lambda: None,
                                   channel_name="Logging UDP")
    if not await login_udp_channel.open(server_ip, login_port):
        return None, "Failed to open login UDP connection!"

    # UDP channel, that will be used for further communication with server
    player_channel = UdpChannel(on_closed_cb=lambda: print("Closed!"),
                                channel_name="Player UDP")
    if not await player_channel.bind(local_addr=(local_ip, local_port)):
        return None, f"Failed to bind main UDP connection on {local_ip}{local_port}!"

    # Creating protobuf channel, that will be used for login procedure (first)
    # and further communication (after Login)
    protobuf_channel = ProtobufChannel(channel_name="Main",
                                       message_type=public.Message)
    protobuf_channel.attach_channel(login_udp_channel)

    # Creating access panel instance and login in
    access_panel = IAccessPanel()
    access_panel.attach_to_channel(protobuf_channel)

    player_port, error = await access_panel.login(
        login=login, password=password,
        local_ip=local_ip, local_port=local_port)
    if error is not None:
        return None, f"Failed to login: {error}"

    player_channel.set_remote(server_ip, player_port)

    # Switch protobuf channel to a new socket + ACTIVE mode
    protobuf_channel.attach_channel(player_channel)
    player_channel.attach_to_terminal(protobuf_channel)
    player_channel.propagate_mode(ChannelMode.ACTIVE)

    # Creating root commutator and attach it to channel
    commutator: Commutator = Commutator(name=f"{local_ip}:{local_port}::Root")
    commutator.attach_channel(protobuf_channel)
    protobuf_channel.attach_to_terminal(commutator)

    return commutator, None