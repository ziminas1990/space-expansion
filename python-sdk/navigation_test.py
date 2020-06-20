import asyncio
import logging.config
from typing import Optional

from expansion.procedures.login import login as login_procedure

from expansion.interfaces.public.commutator import Commutator
from expansion.interfaces.public.ship import Ship
from expansion.interfaces.public.engine import Engine
import expansion.procedures.modules_util as modules_util

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

    ship: Ship = Ship(ship_name=f"{commutator.get_name()}::ship#1")
    error = await commutator.open_tunnel(1, ship)
    if error:
        print(error)

    engine = await modules_util.connect_to_engine(name="engine", ship=ship)
    print(await engine.get_specification())

asyncio.run(main())
