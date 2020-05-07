import asyncio
from transport.udp_channel import UdpChannel
from transport.protobuf_channel import ProtobufChannel

#import protocol.Protocol_pb2 as public
import protocol.Privileged_pb2 as privileged

async def main():
    udp_channel = UdpChannel(on_closed_cb=lambda: print("Closed!"),
                             channel_name="UDP")

    protobuf_channel = ProtobufChannel(name="Player",
                                       toplevel_message_type=privileged.Message)

    protobuf_channel.attach_to_channel(udp_channel)

    if not await udp_channel.open("127.0.0.1", 9999):
        print("Failed to open UDP connection!")
        return


    message = public.Message()
    message.tunnelId = 15
    login_req = message.accessPanel.login
    login_req.login = "login"
    login_req.password = "password"
    login_req.ip = "127.0.0.1"
    login_req.port = 9999
    print(message)

    while True:
        protobuf_channel.send(message)
        response = await protobuf_channel.receive(timeout=2)

        if not response:
            print("Timeout")
        else:
            print(response)
        await asyncio.sleep(2)


asyncio.run(main())