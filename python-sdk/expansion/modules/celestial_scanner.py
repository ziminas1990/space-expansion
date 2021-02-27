from typing import Optional, Callable, Awaitable, List

from expansion.interfaces.public import CelestialScannerI, CelestialScannerSpec
from expansion import utils
from expansion import types
from .base_module import BaseModule


class CelestialScanner(BaseModule):
    def __init__(self,
                 connection_factory: Callable[[], Awaitable[CelestialScannerI]],
                 name: Optional[str] = None):
        super().__init__(connection_factory=connection_factory,
                         logging_name=name or utils.generate_name(CelestialScanner))
        self.specification: Optional[CelestialScannerSpec] = None

    async def get_specification(self, timeout: float = 0.5, reset_cached=False) \
            -> Optional[CelestialScannerSpec]:
        if reset_cached:
            self.specification = None
        if self.specification:
            return self.specification
        async with self._lock_channel() as channel:
            assert isinstance(channel, CelestialScannerI)  # sort of type hinting
            self.specification = await channel.get_specification(timeout)
        return self.specification

    async def scan(self, scanning_radius_km: int, minimal_radius_m: int) \
            -> (Optional[List[types.PhysicalObject]], Optional[str]):
        timeout = 2 * await self.expected_scanning_time(
            scanning_radius_km=scanning_radius_km,
            minimal_radius_m=minimal_radius_m)
        if timeout == 0:
            return 0, "Can't calculate expected timeout"
        if timeout < 0.2:
            timeout = 0.2

        async with self._lock_channel() as channel:
            assert isinstance(channel, CelestialScannerI)  # sort of type hinting
            return await channel.scan_sync(scanning_radius_km, minimal_radius_m, timeout)

    async def expected_scanning_time(self, scanning_radius_km: int, minimal_radius_m: int) -> float:
        """Return time, that should be required by scanner to finish scanning
        request with the specified 'scanning_radius_km' and 'minimal_radius_m'"""
        spec = await self.get_specification()
        if not spec:
            return 0
        resolution = (1000 * scanning_radius_km) / minimal_radius_m
        c_km_per_sec = 300000
        total_processing_time = (resolution * spec.processing_time_us) / 1000000
        return 0.1 + 2 * scanning_radius_km / c_km_per_sec + total_processing_time
