from typing import Optional, Callable, List

from expansion.interfaces.rpc import PassiveScannerI, PassiveScanerSpec
from expansion import utils
from expansion import types
from .base_module import BaseModule, TunnelFactory


ObjectsList = Optional[List[types.PhysicalObject]]
Error = Optional[str]
ScanningCallback = Callable[[ObjectsList, Error], None]


class PassiveScanner(BaseModule):
    def __init__(self,
                 tunnel_factory: TunnelFactory,
                 name: Optional[str] = None):
        super().__init__(tunnel_factory=tunnel_factory,
                         name=name or utils.generate_name(PassiveScanner))
        self.specification: Optional[PassiveScanerSpec] = None

    @BaseModule.use_session(
        terminal_type=PassiveScannerI,
        return_on_unreachable=None,
        return_on_cancel=None)
    async def get_specification(
            self,
            timeout: float = 0.5,
            reset_cached=False,
            session: Optional[PassiveScannerI] = None) \
            -> Optional[PassiveScanerSpec]:
        assert session is not None
        if reset_cached:
            self.specification = None
        if self.specification:
            return self.specification
        self.specification = await session.get_specification(timeout)
        return self.specification

    @BaseModule.use_session_for_generators(
        terminal_type=PassiveScannerI,
        return_on_unreachable=None)
    async def scan(self, session: Optional[PassiveScannerI] = None) \
            -> types.PhysicalObject:
        assert session is not None
        status = await session.start_monitoring()
        if not status.is_success():
            return
        status, objects = await session.wait_update()
        while status.is_success():
            for obj in objects:
                yield obj
            status, objects = await session.wait_update()
