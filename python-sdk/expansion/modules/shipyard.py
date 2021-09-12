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

    async def get_specification(self, timeout: float = 0.5, reset_cached=False) \
            -> Tuple[Status, Optional[ShipyardSpec]]:
        if reset_cached:
            self.specification = None
        if self.specification:
            return ShipyardI.Status.SUCCESS, self.specification
        async with self.rent_session(ShipyardI) as channel:
            assert isinstance(channel, ShipyardI)
            status, self.specification = await channel.get_specification(timeout)
        return status, self.specification

    async def bind_to_cargo(self, cargo_name: str):
        async with self.rent_session(ShipyardI) as channel:
            assert isinstance(channel, ShipyardI)
            return await channel.bind_to_cargo(cargo_name)

    async def build_ship(self, blueprint: str, ship_name: str,
                         progress_cb: Optional[BuildingCallback] = None) -> \
            (Status, Optional[str], Optional[int]):
        Status = ShipyardI.Status
        async with self.open_managed_session(ShipyardI) as building_session:
            if not building_session:
                return ShipyardI.Status.FAILED_TO_SEND_REQUEST, None, None
            assert isinstance(building_session, ShipyardI)

            status = await building_session.start_build(blueprint, ship_name)
            if not status.BUILD_STARTED:
                return status, None, None

            while True:
                status, progress = await building_session.wait_building_report()
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
            return await building_session.wait_building_complete()

    @staticmethod
    def find_by_name(commutator: "Commutator", name: str) -> Optional["Shipyard"]:
        return BaseModule.get_by_name(
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
        
        return await BaseModule.find_best(
            commutator=commutator,
            type=ModuleType.SHIPYARD,
            better_than=better_than
        )
