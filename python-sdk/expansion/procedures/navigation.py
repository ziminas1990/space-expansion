from typing import Callable, Awaitable
import math
import asyncio

from expansion.interfaces.public.engine import Engine, Specification as EngineSpec
from expansion.interfaces.public.ship import Ship
from expansion.interfaces.public.types import Position, Vector


async def move_to(ship: Ship, engine: Engine,
                  position: Callable[[], Awaitable[Position]],
                  sync_interval_ms: int = 250,
                  max_distance_error: float = 1,
                  max_velocity_error: float = 0.1) -> bool:
    """Move the specified 'ship' to the specified 'position' using the
    specified 'engine'. Procedure will complete when a distance between ship
    and 'position' is not more than the specified 'max_distance_error' meters
    AND the ship'd speed is not more than the specified 'max_velocity_error'.
    Procedure will make corrections every 'sync_interval_ms'. Note that the
    higher the 'sunc_interval_ms' value, the lower the accuracy. If the
    required accuracy and sync interval are too high, procedure may never
    complete due to it's non-convergent.
    """
    sync_interval: float = sync_interval_ms / 1000.0

    ship_position = await ship.navigation.get_position()
    ship_state = await ship.get_state()
    target = await position()
    engine_spec: EngineSpec = await engine.get_specification()
    if not ship_position or not ship_state or not target or not engine_spec:
        return False

    ds = ship_position.distance_to(target)
    dv = ship_position.velocity - target.velocity

    while ds > max_distance_error and dv.abs() > max_velocity_error:
        # Calculating the best velocity vector for this moment
        best_velocity = ship_position.vector_to(target)
        best_velocity.set_length(
            math.sqrt(2 * ds * engine_spec.max_thrust / ship_state.weight)
        )

        # Calculating a best thrust for the next time interval
        best_acceleration: Vector = (best_velocity - dv) / sync_interval
        best_thrust: Vector = best_acceleration * ship_state.weight
        if best_thrust.abs() > engine_spec.max_thrust:
            best_thrust.set_length(engine_spec.max_thrust)

        if not engine.set_thrust(thrust=best_thrust, duration_ms=sync_interval_ms):
            return False

        await asyncio.sleep(sync_interval_ms / 1000.0)

        ship_position = await ship.navigation.get_position()
        target = await position()
        if not ship_position or not target:
            return False
        ds = ship_position.distance_to(target)
        dv = ship_position.velocity - target.velocity

    return True