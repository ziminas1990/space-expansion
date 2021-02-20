from typing import Callable, Awaitable, Optional, Tuple
import math
import asyncio

from expansion.types import Position, TimePoint

from expansion import modules
from .base_task import BaseTask


class MoveTo(BaseTask):
    def __init__(self,
                 ship: modules.Ship,
                 target: Callable[[], Awaitable[Position]],
                 system_clock: modules.SystemClock):
        """Create a task, moving the specified 'ship' to the specified 'target'.
        The 'target' is a function (coroutime), that returns current target's
        position"""

        super().__init__(name="MoveTo", time=system_clock.cached_time())
        self.ship: modules.Ship = ship
        self.system_clock: modules.SystemClock = system_clock
        self.target: Callable[[], Awaitable[Position]] = target

    async def _impl(self, max_distance_error=10, max_velocity_error=1) -> bool:
        # Preparations:
        position = await self.ship.get_position(cache_expiring_ms=0)
        ship_state = await self.ship.get_state()
        target = await self.target()

        engine_and_spec = await self._get_most_powerful_engine()
        if engine_and_spec is None:
            self.add_journal_record("can't get engine!")
            return False
        engine, engine_spec = engine_and_spec

        if not position or not ship_state or not target or not engine_spec:
            self.add_journal_record("unknown communication error")
            return False
        max_thrust = engine_spec.max_thrust

        distance = position.distance_to(target)
        relative_v = position.velocity - target.velocity

        # Main loop:
        while distance > max_distance_error or relative_v.abs() > max_velocity_error:
            max_acceleration = max_thrust / ship_state.weight

            # Calculating the best relative velocity vector for this moment
            # Best velocity has such value, that if ship starts slow down right now, it will stop
            # at the required position
            best_velocity = position.vector_to(target)
            best_velocity.set_length(math.sqrt(2 * distance * max_acceleration))

            # dv - how velocity should be changed to become perfect
            dv = best_velocity - relative_v
            burn_time = dv.abs() * ship_state.weight / max_thrust

            # Calculating thrust
            thrust = dv
            thrust.set_length(max_thrust)

            # Heuristic: no sudden movements, please
            if burn_time < 0.5:
                thrust.mult_self(burn_time / 0.5)
                burn_time = 0.5

            if not await engine.set_thrust(thrust=thrust, duration_ms=int(round(burn_time * 1000))):
                return False

            sleep_time = burn_time / 2
            if sleep_time > 2:
                sleep_time = 2
            now = await self.system_clock.wait_for(
                period_us=int(sleep_time * 1000000),
                timeout=2 * sleep_time)
            if now is None:
                # Something went wrong on server (may be lags?)
                return False

            # Getting actual data:
            position, target = await asyncio.gather(
                self.ship.get_position(cache_expiring_ms=0),
                self.target())
            if not position or not target:
                return False
            distance = position.distance_to(target)
            relative_v = position.velocity - target.velocity

        return True

    async def _get_most_powerful_engine(self) \
            -> Optional[Tuple[modules.Engine, modules.EngineSpec]]:
        engines = modules.get_all_engines(self.ship)
        if engines is None:
            return None
        candidate: Optional[Tuple[modules.Engine, modules.EngineSpec]] = None

        for engine in engines:
            spec = await engine.get_specification()
            if spec is not None:
                if candidate is None or candidate[1].max_thrust < spec.max_thrust:
                    candidate = (engine, spec)
        return candidate
