from typing import Callable, Awaitable
import math
import logging

from expansion.types.geometry import Position
from expansion.modules.ship import Ship, ShipState
from expansion.modules.engine import Engine, EngineSpec
import expansion.interfaces.public as rpc


async def move_to(ship: Ship,
                  engine: Engine,
                  position: Callable[[], Awaitable[Position]],
                  system_clock: rpc.SystemClockI,
                  max_distance_error: float = 5,
                  max_velocity_error: float = 0.5,
                  max_thrust_k_limit: float = 1) -> bool:
    """Move the specified 'ship' to the specified 'position' using the
    specified 'engine'. Ths specified 'system_clock' will be used to track the
    server's time. Procedure will complete when a distance between ship
    and 'position' is not more than the specified 'max_distance_error' meters
    AND the ship's speed is not more than the specified 'max_velocity_error'.
    """
    iterations = 0

    ship_position = await ship.get_position(cache_expiring_ms=0)
    ship_state: ShipState = await ship.get_state()
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

        if not await engine.set_thrust(thrust=thrust, duration_ms=int(round(burn_time * 1000))):
            return False

        sleep_time = burn_time / 2
        if sleep_time > 2:
            sleep_time = 2
        now = await system_clock.wait_for(int(sleep_time * 1000000), 2 * sleep_time)
        if now is None:
            # Something went wrong on server (may be lags?)
            return False

        # Getting actual data:
        # TODO: send two requests in parallel!
        ship_position = await ship.get_position(cache_expiring_ms=0)
        target = await position()
        if not ship_position or not target:
            return False
        distance = ship_position.distance_to(target)
        relative_v = ship_position.velocity - target.velocity

    return True
