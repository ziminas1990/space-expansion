import asyncio
import logging.config
from typing import Optional

from expansion.transport.protobuf_channel import ProtobufChannel
from expansion.procedures.login import login as login_procedure

import expansion.protocol.Protocol_pb2 as public
from expansion.interfaces.public.commutator import Commutator

logging.basicConfig(level=logging.DEBUG)

async def main():
    # Creating channels
    commutator: Optional[Commutator] = None
    commutator, error =\
        await login_procedure(server_ip="127.0.0.1", login_port=6842,
                              login="Olenoid", password="admin",
                              local_ip="127.0.0.1", local_port=4435)

    if error:
        print(f"Failed to open UDP connection: {error}")
        return

    modules = await commutator.get_all_modules()
    ship_commutator: Commutator = Commutator("ship")
    error = await commutator.open_tunnel(1, ship_commutator)
    if error:
        print(error)

    print(await ship_commutator.get_all_modules())

asyncio.run(main())
