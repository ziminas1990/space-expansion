import asyncio
import logging.config

from transport.udp_channel import UdpChannel
from transport.protobuf_channel import ProtobufChannel

from protocol.Privileged_pb2 import Message as PrivilegedMessage
from interfaces.privileged.access import Access as PrivilegedAccess


logging.basicConfig(level=logging.DEBUG)


async def main():
    udp_channel = UdpChannel(on_closed_cb=lambda: print("Closed!"),
                             channel_name="UDP")
    if not await udp_channel.open("127.0.0.1", 17392):
        print("Failed to open UDP connection!")
        return

    privileged_channel = ProtobufChannel(name="Player",
                                         toplevel_message_type=PrivilegedMessage)
    privileged_channel.attach_to_channel(udp_channel)

    access_panel = PrivilegedAccess()
    access_panel.attach_to_channel(privileged_channel)

    status, token = await access_panel.login("admin", "admin")
    print(f"{status}: {token}")


asyncio.run(main())
