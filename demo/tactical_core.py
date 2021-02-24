from typing import Optional, Dict
import asyncio

from expansion import modules
from expansion.modules import Commutator, SystemClock
from expansion.types import TimePoint
import logging

from assistants import ShipsAssistant


class TacticalCore:
    """
    This class contains all the useful data about the
    world: all out ships, asteroids, current time and many
    other known things.
    """

    def __init__(self, root_commutator: Commutator):
        self.root_commutator = root_commutator
        self.system_clock: Optional[SystemClock] = None
        self.time = TimePoint(0)
        self.log: logging.Logger = logging.getLogger("TacticalCore")

        self.ships: Dict[str, modules.Ship] = {}
        self.ships_assistant: Optional[ShipsAssistant] = None

    async def initialize(self) -> bool:
        if not await self.root_commutator.init():
            self.log.error("Failed to init root commutator!")
            return False
        self.system_clock = modules.get_system_clock(self.root_commutator)
        if self.system_clock is None:
            self.log.error("SystemClock not found!")
            return False
        self.time = await self.system_clock.time()
        self.system_clock.subscribe(self._on_time_cb)
        self.ships_assistant = ShipsAssistant(root_commutator=self.root_commutator,
                                              system_clock=self.system_clock)
        return True

    async def run(self):
        """Main tactical core loop"""
        while True:
            await self.ships_assistant.sync()
            await asyncio.sleep(0.1)

    def _on_time_cb(self, time: TimePoint):
        self.time = time
