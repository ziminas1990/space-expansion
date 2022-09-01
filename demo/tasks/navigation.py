from typing import Optional

from expansion.types import Position, Vector

from expansion import modules
from expansion.procedures import (
    FlightPlan,
    approach_to_plan,
    follow_flight_plan,
    prepare_flight_plan
)
from .base_task import BaseTask


class MoveTo(BaseTask):
    def __init__(self,
                 ship: modules.Ship,
                 target: Position,
                 system_clock: modules.SystemClock,
                 intercept_course: bool = True,
                 name: str = "MoveTo"):
        """Create a task, moving the specified 'ship' to the specified 'target'.
        The 'target' is a function (coroutine), that returns current target's
        position"""

        super().__init__(name=name, system_clock=system_clock)
        self.ship: modules.Ship = ship
        self.system_clock: modules.SystemClock = system_clock
        self.target: Position = target
        self.intercept_course: bool = intercept_course
        self.plan: Optional[FlightPlan] = None

    async def _impl(self) -> bool:
        engine = await modules.get_most_powerful_engine(self.ship)
        if not engine:
            self.add_journal_record("Can't get engine!")
            return False

        engine_spec = await engine.get_specification()
        ship_state = await self.ship.get_state()
        if not engine_spec or not ship_state:
            self.add_journal_record("Can't get ship's state or engine spec!")
            return False

        amax = engine_spec.max_thrust / ship_state.weight

        # We should switch off engine first
        await engine.set_thrust(Vector(0, 0))

        # Let's figure out where we are going to start
        position: Optional[Position] = await self.ship.get_position(
            at_us=await self.system_clock.time() + 100000   # Take 100 ms for preparations
        )
        if not position:
            return False

        # Building and follow flight plan
        if self.intercept_course and not self.target.velocity.almost_null():
            self.plan = approach_to_plan(
                position=position,
                target=self.target,
                amax=amax
            )
        else:
            self.plan = prepare_flight_plan(
                position=position,
                target=self.target.no_timestamp(),
                amax=amax)
        success = await follow_flight_plan(self.ship, engine, self.plan, self.system_clock)
        self.plan = None
        return success
