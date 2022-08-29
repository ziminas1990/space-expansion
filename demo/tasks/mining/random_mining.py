import asyncio
import random
from typing import Dict, Optional, TYPE_CHECKING
from tasks.base_task import BaseTask
from tasks.mining.simple_mining import SimpleMining
import expansion.modules as modules

if TYPE_CHECKING:
    from ship import Ship
    from expansion.modules import SystemClock
    from expansion.types import PhysicalObject
    from tactical_core import TacticalCore


class RandomMining(BaseTask):

    def __init__(self,
                 name: str,
                 tactical_core: "TacticalCore",
                 warehouse: "Ship",
                 system_clock: "SystemClock"):
        super().__init__(name, system_clock)
        self.tactical_core = tactical_core
        self.warehouse = warehouse

        self._tasks: Dict["Ship", Optional[SimpleMining]] = {}

    # Check if the specified 'candidate' ship is sufficiently equipped
    # for mining task
    @staticmethod
    def can_use_ship(candidate: "Ship") -> bool:
        print(f"GREPIT: check if ship {candidate.name} is suitable for mining")
        return candidate.has_modules(
            [modules.ModuleType.RESOURCE_CONTAINER,
             modules.ModuleType.ASTEROID_MINER,
             modules.ModuleType.ENGINE])

    def add_ship(self, miner: "Ship"):
        if miner not in self._tasks:
            self._tasks.update({miner: None})

    async def _impl(self, *argc, **argv) -> bool:
        cycle = 1
        while True:
            for miner, task in self._tasks.items():
                if task and not task.finished:
                    continue
                asteroid = self._choose_random_asteroid()
                if not asteroid:
                    break
                self.add_journal_record(f"Asteroid {asteroid.object_id} is "
                                        f"chosen for '{miner.name}'")
                task = SimpleMining(
                    name=f"{self.name}.{miner.name}.turn_{cycle}",
                    tactical_core=self.tactical_core,
                    ship=miner,
                    asteroid_id=asteroid.object_id,
                    warehouse=self.warehouse,
                    system_clock=self.system_clock)
                self._tasks.update({miner: task})
                task.run_async()
            await asyncio.sleep(0.5)
        return True

    def _choose_random_asteroid(self) -> Optional["PhysicalObject"]:
        if not self.tactical_core.world.asteroids:
            return None
        attempts = len(self.tactical_core.world.asteroids).bit_length()
        position = self.warehouse.predict_position()

        all_asteroids = list(self.tactical_core.world.asteroids.values())
        best = random.choice(all_asteroids)
        for attempt in range(attempts):
            asteroid = random.choice(all_asteroids)
            if position.distance_to(asteroid.position) < position.distance_to(best.position):
                best = asteroid
        return best
