import asyncio
import logging.config

from expansion.transport.udp_channel import UdpChannel
from expansion.transport.protobuf_channel import ProtobufChannel

from expansion.protocol.Privileged_pb2 import Message as PrivilegedMessage
from expansion.interfaces.privileged.access import Access as PrivilegedAccess
from expansion.interfaces.privileged.screen import Screen as PrivilegedScreen
import expansion.interfaces.privileged.types as privileged_types


logging.basicConfig(level=logging.DEBUG)


async def main():

    # Creating channels
    udp_channel = UdpChannel(on_closed_cb=lambda: print("Closed!"),
                             channel_name="UDP")
    if not await udp_channel.open("127.0.0.1", 17392):
        print("Failed to open UDP connection!")
        return

    privileged_channel = ProtobufChannel(name="Player",
                                         toplevel_message_type=PrivilegedMessage)
    privileged_channel.attach_to_channel(udp_channel)

    # Creating components:
    access_panel = PrivilegedAccess()
    access_panel.fasten_to_channel(privileged_channel)

    # Logging in
    status, token = await access_panel.login("admin", "admin")
    screen = PrivilegedScreen()
    screen.attach_to_channel(channel=privileged_channel, token=token)

    if not await screen.set_position(center_x=100000, center_y=0, width=50000, height=50000):
        return

    objects = await screen.show(privileged_types.ObjectType.ASTEROID)
    print(objects)

asyncio.run(main())
