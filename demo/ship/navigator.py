import asyncio
from typing import Optional
from enum import Enum

from expansion import modules
from expansion import types

from tasks.navigation import MoveTo


class Navigator:
    def __init__(self, name: str, ship: modules.Ship, system_clock: modules.SystemClock):
        self.name = name
        self.ship = ship
        self.system_clock = system_clock

        self.task_move_to: Optional[MoveTo] = None

    def interrupt(self):
        if self.task_move_to:
            self.task_move_to.interrupt()

    def move_to_async(self, position: types.Position, intercept_course: bool = True) -> bool:
        self.interrupt()
        self.task_move_to = self.__move_to_task(position, intercept_course)
        self.task_move_to.run_async(complete_cb=self.__move_to_finished)
        return True

    async def move_to(self, position: types.Position, intercept_course: bool = True) -> bool:
        self.interrupt()
        self.task_move_to = self.__move_to_task(position, intercept_course)
        return await self.task_move_to.run()

    def __move_to_task(self, position: types.Position, intercept_course: bool = True):
        return MoveTo(
            ship=self.ship,
            target=position,
            system_clock=self.system_clock,
            intercept_course=intercept_course,
            name=f"{self.name}/MovingTo")

    def __move_to_finished(self, status: bool):
        pass
