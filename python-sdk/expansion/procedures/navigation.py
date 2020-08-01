from typing import Callable, Awaitable
import math
import asyncio

from expansion.interfaces.public.engine import Engine, Specification as EngineSpec
from expansion.interfaces.public.ship import Ship
from expansion.interfaces.public.types import Position


async def move_to(ship: Ship, engine: Engine,
                  position: Callable[[], Awaitable[Position]],
                  max_distance_error: float = 5,
                  max_velocity_error: float = 0.5,
                  max_thrust_k_limit: float = 1,
                  cb_sleep: Callable[[float], Awaitable[None]] = asyncio.sleep) -> bool:
    """Move the specified 'ship' to the specified 'position' using the
    specified 'engine'. Procedure will complete when a distance between ship
    and 'position' is not more than the specified 'max_distance_error' meters
    AND the ship's speed is not more than the specified 'max_velocity_error'.
    The specified 'cb_sleep' will be called every time, when procedure needs to
    await for some in-game time
    """
    iterations = 0

    ship_position = await ship.navigation.get_position()
    ship_state = await ship.get_state()
    target = await position()
    engine_spec: EngineSpec = await engine.get_specification()
    if not ship_position or not ship_state or not target or not engine_spec:
        return False
    engine_max_thrust = engine_spec.max_thrust * max_thrust_k_limit

    distance = ship_position.distance_to(target)
    relative_v = ship_position.velocity - target.velocity

    while distance > max_distance_error or relative_v.abs() > max_velocity_error:
        iterations += 1
        max_acceleration = engine_max_thrust / ship_state.weight

        # Calculating the best relative velocity vector for this moment
        # Best velocity has such value, that if ship starts slow down right now, it will stop
        # at the required position
        best_velocity = ship_position.vector_to(target)
        best_velocity.set_length(math.sqrt(2 * distance * max_acceleration))

        # dv - how velocity should be changed to become perfect
        dv = best_velocity - relative_v
        burn_time = dv.abs() * ship_state.weight / engine_max_thrust

        # Calculating thrust
        thrust = dv
        thrust.set_length(engine_max_thrust)

        # Heuristic: no sudden movements, please
        if burn_time < 0.5:
            thrust.mult_self(burn_time / 0.5)
            burn_time = 0.5

        if not engine.set_thrust(thrust=thrust, duration_ms=int(round(burn_time * 1000))):
            return False

        sleep_time = burn_time / 2
        if sleep_time > 2:
            sleep_time = 2
        await cb_sleep(sleep_time)

        # Getting actual data:
        # TODO: send to requests in parallel!
        ship_position = await ship.navigation.get_position()
        target = await position()
        if not ship_position or not target:
            return False
        distance = ship_position.distance_to(target)
        relative_v = ship_position.velocity - target.velocity

    return True
