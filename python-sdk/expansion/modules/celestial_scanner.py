from typing import Optional, Callable, Awaitable, List

from expansion.interfaces.rpc import CelestialScannerI, CelestialScannerSpec
from expansion import utils
from expansion import types
from .base_module import BaseModule, TunnelFactory


ObjectsList = Optional[List[types.PhysicalObject]]
Error = Optional[str]
ScanningCallback = Callable[[ObjectsList, Error], None]


class CelestialScanner(BaseModule):
    def __init__(self,
                 tunnel_factory: TunnelFactory,
                 name: Optional[str] = None):
        super().__init__(tunnel_factory=tunnel_factory,
                         name=name or utils.generate_name(CelestialScanner))
        self.specification: Optional[CelestialScannerSpec] = None

    @BaseModule.use_session(
        terminal_type=CelestialScannerI,
        return_on_unreachable=None,
        return_on_cancel=None)
    async def get_specification(
            self,
            timeout: float = 0.5,
            reset_cached=False,
            session: Optional[CelestialScannerI] = None) \
            -> Optional[CelestialScannerSpec]:
        assert session is not None
        if reset_cached:
            self.specification = None
        if self.specification:
            return self.specification
        self.specification = await session.get_specification(timeout)
        return self.specification

    @BaseModule.use_session(
        terminal_type=CelestialScannerI,
        return_on_unreachable="Channel was closed",
        return_on_cancel="Operation was canceled")
    async def scan(self,
                   scanning_radius_km: int,
                   minimal_radius_m: int,
                   scanning_cb: ScanningCallback,
                   session: Optional[CelestialScannerI] = None) \
            -> Optional[str]:
        """Scanning all bodies within a 'scanning_radius_km' radius.

        Bodies with radius less then the specified 'minimal_radius_m' will be
        ignored. The 'scanning_cb' callback will be called when another portion
        of data received from the server. After all data received callback
        will be called for the last time with (None, None) argument.
        Return None on success otherwise return error string
        """
        assert session is not None
        timeout = 2 * await self.expected_scanning_time(
            scanning_radius_km=scanning_radius_km,
            minimal_radius_m=minimal_radius_m,
            session=session)
        if timeout == 0:
            return "Can't calculate expected timeout"
        if timeout < 0.2:
            timeout = 0.2

        return await session.scan(
            scanning_radius_km,
            minimal_radius_m,
            scanning_cb,
            timeout)

    @BaseModule.use_session(
        terminal_type=CelestialScannerI,
        return_on_unreachable=(None, "Channel was closed"),
        return_on_cancel=(None, "Operation was canceled"))
    async def scan_sync(self,
                        scanning_radius_km: int,
                        minimal_radius_m: int,
                        session: CelestialScannerI = None) \
            -> (ObjectsList, Error):
        """Same as 'scan()' but without user's callback"""
        assert session is not None
        timeout = 2 * await self.expected_scanning_time(
            scanning_radius_km=scanning_radius_km,
            minimal_radius_m=minimal_radius_m,
            session=session)
        if timeout == 0:
            return 0, "Can't calculate expected timeout"
        if timeout < 0.2:
            timeout = 0.2

        return await session.scan_sync(
            scanning_radius_km,
            minimal_radius_m,
            timeout)

    @BaseModule.use_session(
        terminal_type=CelestialScannerI,
        return_on_unreachable=0,
        return_on_cancel=0)
    async def expected_scanning_time(self,
                                     scanning_radius_km: int,
                                     minimal_radius_m: int,
                                     session: CelestialScannerI = None) -> float:
        """Return time, that should be required by scanner to finish scanning
        request with the specified 'scanning_radius_km' and 'minimal_radius_m'"""
        assert session is not None
        spec = await self.get_specification(session=session)
        if not spec:
            return 0
        resolution = (1000 * scanning_radius_km) / minimal_radius_m
        c_km_per_sec = 300000
        total_processing_time = (resolution * spec.processing_time_us) / 1000000
        return 0.1 + 2 * scanning_radius_km / c_km_per_sec + total_processing_time
