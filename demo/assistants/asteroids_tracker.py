
from typing import Dict, List, Optional, NamedTuple, Tuple
import asyncio
import logging

from expansion import modules, types
from expansion.modules.util import get_all_ships, get_any_celestial_scanner
from world import World


class ScanningParams(NamedTuple):
    radius_km: int
    asteroid_radius_m: int


class AsteroidTracker:
    _logger = logging.getLogger(__name__)

    def __init__(self,
                 root_commutator: modules.Commutator,
                 world: World,
                 system_clock: modules.SystemClock):
        self.root_commutator: modules.Commutator = root_commutator
        self.system_clock = system_clock
        self.world: World = world

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

    async def rescan_asteroid(self,
                              asteroid: types.PhysicalObject,
                              max_distance_km: float = 1000) -> bool:
        now_us = self.system_clock.cached_time()
        # Looking for a scanner nearby
        scanner, distance = self._find_scanner_nearby(asteroid.position.predict(now_us), now_us)
        if not scanner or (distance / 1000 > max_distance_km):
            return False
        # Scanning
        scanning_distance_km: int = int(1.4 * distance / 1000)
        result, error = await scanner.scan_sync(
            scanning_radius_km=min(scanning_distance_km, 10),
            minimal_radius_m=5+int(asteroid.radius))

        previous_timestamp = asteroid.position.timestamp
        self.__on_result(result, error)
        # Check that asteroid has been updated actually
        return asteroid.position.timestamp > previous_timestamp

    def run_auto_scanning(self, scanning_modes: List[ScanningParams]):
        self.auto_scanning_modes = scanning_modes
        asyncio.get_running_loop().create_task(self.__auto_scanning())

    def stop_auto_scanning(self):
        self.auto_scanning_modes = []

    def __on_result(
            self,
            objects: Optional[List[types.PhysicalObject]],
            error: Optional[str]):
        if error is not None:
            self._logger.warning(f"Scanning failed: {error}")
            return
        if objects is None:
            return
        self.world.update_asteroids(objects)

    async def __auto_scanning(self):
        while len(self.auto_scanning_modes):
            idle = True
            for ship in get_all_ships(self.root_commutator):
                scanner = get_any_celestial_scanner(ship)
                if not scanner:
                    continue
                for mode in self.auto_scanning_modes:
                    idle = False
                    await scanner.scan(
                        scanning_radius_km=mode.radius_km,
                        minimal_radius_m=mode.asteroid_radius_m,
                        scanning_cb=self.__on_result
                    )
            if idle:
                await asyncio.sleep(0.5)

    def _find_scanner_nearby(self,
                             position: types.Position,
                             now_us: Optional[int] = None) -> Tuple[modules.CelestialScanner, float]:
        best_scanner: Optional[modules.CelestialScanner] = None
        best_distance = 0
        for ship in get_all_ships(self.root_commutator):
            scanner = get_any_celestial_scanner(ship)
            if not scanner:
                continue
            distance = ship.position.predict(now_us).distance_to(position)
            if best_scanner is None or distance < best_distance:
                best_distance = distance
                best_scanner = scanner
        return best_scanner, best_distance
