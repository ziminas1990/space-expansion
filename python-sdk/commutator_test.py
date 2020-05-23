import asyncio
import logging.config
from typing import Optional

from expansion.transport.udp_channel import UdpChannel
from expansion.transport.protobuf_channel import ProtobufChannel
from expansion.transport.inversed_channel import InversedChannel

import expansion.protocol.Protocol_pb2 as public
from expansion.interfaces.public.IAccessPanel import IAccessPanel
from expansion.interfaces.public.commutator import Commutator

logging.basicConfig(level=logging.DEBUG)


async def login(server_ip: str, login_port: int,
                login: str, password: str,
                local_ip: str, local_port: int) -> (UdpChannel, str):
    # UDP channel, that will be used to login:
    login_udp_channel = UdpChannel(on_closed_cb=lambda: print("Closed!"),
                                   channel_name="Logging UDP")

    # Connecting to server and create AccessPanel instance
    if not await login_udp_channel.open(server_ip, login_port):
        return None, "Failed to open login UDP connection!"

    # Protobuf channel for logging
    protobuf_channel = ProtobufChannel(name="Login",
                                       toplevel_message_type=public.Message)
    protobuf_channel.attach_to_channel(login_udp_channel)

    # Creating access panel instance
    access_panel = IAccessPanel()
    access_panel.attach_to_channel(protobuf_channel)

    # UDP channel, that will be used to further communication with server
    # (will be returned if login succeed)
    player_channel = UdpChannel(on_closed_cb=lambda: print("Closed!"),
                                channel_name="Player UDP")
    if not await player_channel.bind(local_addr=(local_ip, local_port)):
        return None, f"Failed to bind main UDP connection on {local_ip}{local_port}!"

    # Logging in...
    player_port, error = await access_panel.login(
        login=login, password=password,
        local_ip=local_ip, local_port=local_port)
    if error is not None:
        return None, f"Failed to login: {error}"

    # Set up player's channel and return
    player_channel.set_remote(server_ip, player_port)
    return player_channel, None

async def main():
    # Creating channels
    udp_channel, error = await login(server_ip="127.0.0.1", login_port=6842,
                                     login="Olenoid", password="admin",
                                     local_ip="127.0.0.1", local_port=4435)
    if error:
        print(f"Failed to open UDP connection: {error}")
        return

    public_channel = ProtobufChannel(name="Player",
                                     toplevel_message_type=public.Message)
    public_channel.attach_to_channel(udp_channel)

    inversed_channel: InversedChannel = InversedChannel(public_channel)
    asyncio.create_task(inversed_channel.run())

    commutator: Commutator = Commutator()
    commutator.attach_channel(inversed_channel)
    inversed_channel.attach_to_terminal(commutator)
    total_slots = await commutator.get_total_slots()
    print(total_slots)

asyncio.run(main())
