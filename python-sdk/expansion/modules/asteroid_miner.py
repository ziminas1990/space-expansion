from typing import Optional, Callable, Awaitable, Tuple, TYPE_CHECKING

from expansion.interfaces.rpc import AsteroidMinerI, AsteroidMinerSpec
from expansion import utils
from expansion.modules import ModuleType, BaseModule
from expansion.transport import Channel

if TYPE_CHECKING:
    from expansion.modules import Commutator, TunnelFactory

Comparator = Callable[[AsteroidMinerSpec, AsteroidMinerSpec], bool]


class AsteroidMiner(BaseModule):

    Status = AsteroidMinerI.Status

    def __init__(self,
                 tunnel_factory: "TunnelFactory",
                 name: Optional[str] = None):
        super().__init__(tunnel_factory=tunnel_factory,
                         name=name or utils.generate_name(AsteroidMiner))
        self.specification: Optional[AsteroidMinerSpec] = None
        self.cargo_name: Optional[str] = None
        # A name of the resource container, to which the miner is attached

    @BaseModule.use_session(
        terminal_type=AsteroidMinerI,
        return_on_unreachable=(Status.FAILED_TO_SEND_REQUEST, None),
        return_on_cancel=(Status.CANCELED, None))
    async def get_specification(
            self,
            timeout: float = 0.5,
            reset_cached=False,
            session: Optional[AsteroidMinerI] = None) \
            -> Tuple[Status, Optional[AsteroidMinerSpec]]:
        assert session is not None
        if reset_cached:
            self.specification = None
        if self.specification:
            return AsteroidMinerI.Status.SUCCESS, self.specification
        status, self.specification = await session.get_specification(timeout)
        return status, self.specification

    @BaseModule.use_session(
        terminal_type=AsteroidMinerI,
        return_on_unreachable=Status.FAILED_TO_SEND_REQUEST,
        return_on_cancel=Status.CANCELED)
    async def bind_to_cargo(
            self,
            cargo_name: str,
            timeout: float = 0.5,
            session: Optional[AsteroidMinerI] = None) -> Status:
        """Bind miner to the container with the specified 'cargo_name'"""
        assert session is not None
        status = await session.bind_to_cargo(cargo_name, timeout)
        if status == AsteroidMinerI.Status.SUCCESS:
            self.cargo_name = cargo_name
        return status

    @BaseModule.use_session(
        terminal_type=AsteroidMinerI,
        return_on_unreachable=Status.FAILED_TO_SEND_REQUEST,
        return_on_cancel=Status.CANCELED,
        close_after_use=True)
    async def start_mining(
            self,
            asteroid_id: int,
            progress_cb: AsteroidMinerI.MiningReportCallback,
            session: Optional[AsteroidMinerI] = None)\
            -> Status:
        """Start mining asteroid with the specified 'asteroid_id'.
        The specified 'progress_cb' will be called in the following cases:
        1. the mining report is received from server - in this case a
           'SUCCESS' status will be passed as the first argument, and a total
           retrieved resources (since last call) will be passed as the second
           argument. If callback returns False, then mining process will be
           interrupted, otherwise it will be resumed.
        3. some error occurred during mining - in this case an error code (status)
           will be passed as the first argument, and 'None' as the second argument;
           in this case a value, returned by callback, will be ignored and the mining
           process will be interrupted.
        """
        assert session is not None
        return await session.start_mining(asteroid_id, progress_cb)

    @BaseModule.use_session(
        terminal_type=AsteroidMinerI,
        return_on_unreachable=Status.FAILED_TO_SEND_REQUEST,
        return_on_cancel=Status.CANCELED)
    async def stop_mining(self,
                          timeout: float = 0.5,
                          session: Optional[AsteroidMinerI] = None) -> Status:
        """Stop the mining process"""
        assert session is not None
        return await session.stop_mining(timeout)

    @staticmethod
    def find_by_name(commutator: "Commutator", name: str) -> Optional["AsteroidMiner"]:
        return BaseModule._get_by_name(
            commutator=commutator,
            type=ModuleType.ASTEROID_MINER,
            name=name
        )

    @staticmethod
    async def find_most_efficient(commutator: "Commutator") -> Optional["AsteroidMiner"]:
        async def better_than(candidate: "AsteroidMiner",
                              best: "AsteroidMiner"):
            _, best_spec = await best.get_specification()
            if not best_spec:
                return False
            _, spec = await candidate.get_specification()
            if not spec:
                return False
            best_k = best_spec.yield_per_cycle / best_spec.cycle_time_ms
            k = spec.yield_per_cycle / spec.cycle_time_ms
            return k > best_k

        return await BaseModule._find_best(
            commutator=commutator,
            type=ModuleType.ASTEROID_MINER,
            better_than=better_than)
