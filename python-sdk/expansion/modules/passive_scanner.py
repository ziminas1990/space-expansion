from typing import Optional, Callable, List, AsyncIterable, TYPE_CHECKING

from expansion.interfaces.rpc import PassiveScannerI, PassiveScanerSpec
from expansion import utils
from expansion import types
from .base_module import BaseModule, ModuleType, TunnelFactory

if TYPE_CHECKING:
    from expansion.modules import Commutator


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
            -> AsyncIterable[types.PhysicalObject]:
        assert session is not None
        status = await session.start_monitoring()
        if not status.is_success():
            return
        status, objects = await session.wait_update()
        while status.is_success():
            for obj in objects:
                yield obj
            status, objects = await session.wait_update()

    @staticmethod
    def get_by_name(commutator: "Commutator", name: str) \
            -> Optional["PassiveScanner"]:
        return BaseModule._get_by_name(
            commutator=commutator,
            type=ModuleType.PASSIVE_SCANNER,
            name=name
        )

    @staticmethod
    async def get_most_ranged(commutator: "Commutator") \
            -> Optional["PassiveScanner"]:
        async def better_than(candidate: "PassiveScanner",
                              best: "PassiveScanner"):
            best_spec = await best.get_specification()
            if not best_spec:
                return False
            candidate_spec = await candidate.get_specification()
            if not candidate_spec:
                return False
            return candidate_spec.scanning_radius_km > \
                   best_spec.scanning_radius_km

        # Return a scanner, that has the greates scanning range
        return await BaseModule._find_best(
            commutator=commutator,
            type=ModuleType.PASSIVE_SCANNER,
            better_than=better_than
        )
