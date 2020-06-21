import asyncio
import logging.config
from typing import Optional

from expansion.procedures.login import login as login_procedure

from expansion.interfaces.public.commutator import Commutator
from expansion.interfaces.public.ship import Ship
from expansion.interfaces.public.engine import Engine
from expansion.interfaces.public.types import Position, Vector
import expansion.procedures.modules_util as modules_util
import expansion.procedures.navigation as navigation

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

    engine: Engine = await modules_util.connect_to_engine(name="engine", ship=ship)
    target = Position(x=-1000, y=-2000, velocity=Vector(x=0, y=0))

    async def get_target():
        return target

    status, iterations = await navigation.move_to(
        ship=ship, engine=engine, position=get_target,
        max_distance_error=5,
        max_velocity_error=2,
        max_thrust_k_limit=1)
    position = await ship.get_navigation().get_position()
    await asyncio.sleep(0.5)
    print(position)
    print(f"{status}, iterations = {iterations}")
    await asyncio.sleep(2)

    status, iterations = await navigation.move_to(
        ship=ship, engine=engine, position=get_target,
        max_distance_error=1,
        max_velocity_error=0.1,
        max_thrust_k_limit=0.1)
    position = await ship.get_navigation().get_position()
    await asyncio.sleep(0.5)
    print(position)
    print(f"{status}, iterations = {iterations}")
    await asyncio.sleep(2)

asyncio.run(main())
