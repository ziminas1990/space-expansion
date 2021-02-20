from typing import Optional

from expansion import modules
from expansion import types

import tasks


class ShipComputer:
    def __init__(self, ship: modules.Ship, system_clock: modules.SystemClock):
        self.ship = ship
        self.system_clock = system_clock
        self.navigation_task: Optional[tasks.BaseTask] = None

    def move_to(self, x: float, y: float):
        if self.navigation_task is not None:
            self.navigation_task.interrupt()

        async def _target():
            return types.Position(x=x, y=y, velocity=types.Vector(0, 0))

        self.navigation_task = tasks.MoveTo(
            ship=self.ship,
            target=_target,
            system_clock=self.system_clock)
        self.navigation_task.run()
