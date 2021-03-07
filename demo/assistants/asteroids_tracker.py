
from typing import Dict, List, Optional, NamedTuple
import asyncio

from expansion.types import PhysicalObject
from expansion import modules
from expansion.modules.util import get_all_ships, get_any_celestial_scanner


class ScanningParams(NamedTuple):
    radius_km: int
    asteroid_radius_m: int


class AsteroidTracker:

    def __init__(self, root_commutator: modules.Commutator):
        self.root_commutator: modules.Commutator = root_commutator
        self.asteroids: Dict[int, PhysicalObject] = {}

        # Auto scanning data:
        self.auto_scanning_modes: List[ScanningParams] = []

    async def scan_global(self,
                          scanning_radius_km: int,
                          minimal_asteroid_radius_m: int):
        """Scanning using all available scanners"""
        scanners = []
        for ship in get_all_ships(self.root_commutator):
            scanner = get_any_celestial_scanner(ship)
            if scanner:
                scanners.append(scanner)
        await asyncio.gather(*[
            scanner.scan(
                scanning_radius_km,
                minimal_asteroid_radius_m,
                self.__on_result)
            for scanner in scanners])

    def run_auto_scanning(self, scanning_modes: List[ScanningParams]):
        self.auto_scanning_modes = scanning_modes
        asyncio.get_running_loop().create_task(self.__auto_scanning())

    def stop_auto_scanning(self):
        self.auto_scanning_modes = []

    def __on_result(
            self,
            objects: Optional[List[PhysicalObject]],
            error: Optional[str]):
        if not objects:
            return

        for asteroid in objects:
            if asteroid.object_id not in self.asteroids:
                print(f"New asteroid detected: r = {asteroid.radius}m")
            self.asteroids[asteroid.object_id] = asteroid

    async def __auto_scanning(self):
        while len(self.auto_scanning_modes):
            for ship in get_all_ships(self.root_commutator):
                scanner = get_any_celestial_scanner(ship)
                if not scanner:
                    continue
                for mode in self.auto_scanning_modes:
                    print(f"Scanning in {mode.radius_km} km for asteroids > "
                          f"{mode.asteroid_radius_m} m")
                    await scanner.scan(
                        scanning_radius_km=mode.radius_km,
                        minimal_radius_m=mode.asteroid_radius_m,
                        scanning_cb=self.__on_result
                    )
