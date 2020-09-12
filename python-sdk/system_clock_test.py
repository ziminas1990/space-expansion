import asyncio

from expansion.transport.udp_channel import UdpChannel
from expansion.transport.protobuf_channel import ProtobufChannel

from expansion.protocol.Privileged_pb2 import Message as PrivilegedMessage
from expansion.interfaces.privileged.access import Access as PrivilegedAccess
from expansion.interfaces.privileged.system_clock import SystemClock


#logging.basicConfig(level=logging.DEBUG)


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
    access_panel.attach_to_channel(privileged_channel)

    # Logging in
    status, token = await access_panel.login("admin", "admin")

    clock: SystemClock = SystemClock()
    clock.attach_to_channel(privileged_channel, token)

    for i in range(9):
        time: int = await clock.get_time()
        mode = await clock.get_mode()
        print(f"{mode}: {time} us")
        await asyncio.sleep(0.33333)

    assert await clock.switch_to_debug_mode()

    time: int = await clock.get_time()
    mode = await clock.get_mode()
    print(f"{mode}: {time} us")

    await clock.set_tick_duration(1000)
    await clock.proceed_ticks(100000, timeout_sec=10)

    time: int = await clock.get_time()
    mode = await clock.get_mode()
    print(f"{mode}: {time} us")
    await asyncio.sleep(1)

    assert await clock.switch_to_real_time()

    for i in range(9):
        time: int = await clock.get_time()
        mode = await clock.get_mode()
        print(f"{mode}: {time} us")
        await asyncio.sleep(0.33333)

    assert await clock.terminate()

asyncio.run(main())
