from typing import Optional, Tuple, Callable, TYPE_CHECKING

from expansion import utils
from expansion.modules import BaseModule, ModuleType
from expansion.interfaces.rpc import ShipyardI, ShipyardSpec
if TYPE_CHECKING:
    from expansion.modules import Commutator
    from expansion.modules.base_module import TunnelFactory


class Shipyard(BaseModule):

    Status = ShipyardI.Status

    BuildingCallback = Callable[[Status, float], None]

    def __init__(self,
                 tunnel_factory: "TunnelFactory",
                 name: Optional[str] = None):
        super().__init__(tunnel_factory=tunnel_factory,
                         name=name or utils.generate_name(Shipyard))
        self.specification: Optional[ShipyardSpec] = None

    @BaseModule.use_session(
        terminal_type=ShipyardI,
        return_on_unreachable=(Status.FAILED_TO_SEND_REQUEST, None),
        return_on_cancel=(Status.CANCELED, None))
    async def get_specification(self,
                                timeout: float = 0.5,
                                reset_cached=False,
                                session: Optional[ShipyardI] = None) \
            -> Tuple[Status, Optional[ShipyardSpec]]:
        assert session is not None
        if reset_cached:
            self.specification = None
        if self.specification:
            return ShipyardI.Status.SUCCESS, self.specification
        status, self.specification = await session.get_specification(timeout)
        return status, self.specification

    @BaseModule.use_session(
        terminal_type=ShipyardI,
        return_on_unreachable=Status.FAILED_TO_SEND_REQUEST,
        return_on_cancel=Status.CANCELED)
    async def bind_to_cargo(self,
                            cargo_name: str,
                            session: Optional[ShipyardI] = None) \
            -> Status:
        assert session is not None
        return await session.bind_to_cargo(cargo_name)

    @BaseModule.use_session(
        terminal_type=ShipyardI,
        close_after_use=True,
        return_on_unreachable=(Status.FAILED_TO_SEND_REQUEST, None, None),
        return_on_cancel=(Status.CANCELED, None, None))
    async def build_ship(self,
                         blueprint: str,
                         ship_name: str,
                         progress_cb: Optional[BuildingCallback] = None,
                         session: Optional[ShipyardI] = None) -> \
            (Status, Optional[str], Optional[int]):
        assert session is not None
        Status = ShipyardI.Status
        status = await session.start_build(blueprint, ship_name)
        if not status.BUILD_STARTED:
            return status, None, None

        while True:
            status, progress = await session.wait_building_report()
            if progress_cb:
                progress_cb(status, progress)
            if status in {Status.BUILD_IN_PROGRESS, Status.BUILD_FROZEN}:
                # Just waiting for next message
                continue
            elif status == ShipyardI.Status.BUILD_COMPLETE:
                break
            # Seems that some error occurred
            return status, None, None

        # Got report with COMPLETE status. Waiting for a final
        # 'building_complete' message
        # Waiting for building reports
        return await session.wait_building_complete()

    @staticmethod
    def find_by_name(commutator: "Commutator", name: str) -> Optional["Shipyard"]:
        return BaseModule._get_by_name(
            commutator=commutator,
            type=ModuleType.SHIPYARD,
            name=name
        )

    @staticmethod
    async def find_most_productive(commutator: "Commutator") -> Optional["Shipyard"]:
        async def better_than(candidate: "Shipyard",
                              best: "Shipyard"):
            _, best_spec = await best.get_specification()
            if not best_spec:
                return False
            _, spec = await candidate.get_specification()
            if not spec:
                return False
            return spec.labor_per_sec > best_spec.labor_per_sec
        
        return await BaseModule._find_best(
            commutator=commutator,
            type=ModuleType.SHIPYARD,
            better_than=better_than
        )
