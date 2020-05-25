import asyncio
import logging.config
from typing import Optional

from expansion.transport.protobuf_channel import ProtobufChannel
from expansion.transport.inversed_channel import InversedChannel
from expansion.procedures.login import login as login_procedure

import expansion.protocol.Protocol_pb2 as public
from expansion.interfaces.public.commutator import RootCommutator

logging.basicConfig(level=logging.DEBUG)

async def main():
    # Creating channels
    commutator: Optional[RootCommutator] = None
    commutator, error =\
        await login_procedure(server_ip="127.0.0.1", login_port=6842,
                              login="Olenoid", password="admin",
                              local_ip="127.0.0.1", local_port=4435)

    if error:
        print(f"Failed to open UDP connection: {error}")
        return

    asyncio.create_task(commutator.run())

    total_slots = await commutator.get_total_slots()
    print(total_slots)
    await commutator.stop()

asyncio.run(main())
