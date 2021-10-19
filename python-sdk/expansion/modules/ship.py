import asyncio
from typing import Optional, TYPE_CHECKING

import expansion.interfaces.rpc as rpc
from expansion.types import Position
from expansion.modules import BaseModule, ModuleType
from .commutator import Commutator

if TYPE_CHECKING:
    from expansion.modules.base_module import TunnelFactory
    from .commutator import ModulesFactory


class Ship(Commutator, BaseModule):
    """This class represent a wrapper for the remote ship. Is supports
    caching and prediction ship's position"""

    def __init__(self,
                 ship_type: str,
                 ship_name: str,
                 modules_factory: "ModulesFactory",
                 tunnel_factory: "TunnelFactory"):
        super().__init__(
            tunnel_factory=tunnel_factory,
            modules_factory=modules_factory,
            name=ship_name)
        self.type = ship_type
        self.name = ship_name

        self.position: Optional[Position] = None
        self.state: Optional[rpc.ShipState] = None

        # Monitoring facilities
        self.__monitoring_task: Optional[asyncio.Task] = None

    async def init(self) -> bool:
        return await super().init()

    @BaseModule.use_session(
        terminal_type=rpc.ShipI,
        return_on_unreachable=False,
        return_on_cancel=False)
    async def sync(self,
                   timeout: float = 0.5,
                   session: Optional[rpc.ShipI] = None) -> bool:
        """Update ship's information."""
        assert session is not None
        position = await session.navigation().get_position(timeout=timeout)
        if position is not None:
            self.position = position
            return True
        else:
            return False

    async def get_position(self,
                           at_us: Optional[int] = None,
                           cache_expiring_ms: int = 10,
                           timeout: float = 0.5) -> Optional[Position]:
        """Get the current ship's position. If the position was cached more than the
        specified 'cache_expiring_ms' milliseconds ago, than it will be updated.
        Otherwise a cached position will be returned.
        If updating position is required (cache has expired), the request will be
        sent to the server and this call will block control until the response is
        received or the specified 'timeout' occurs.
        """
        if self.position is None:
            if not await self.sync(timeout=timeout):
                return None
            assert self.position is not None
            return self.position.predict(at_us) if at_us else self.position

        if self.position.expired(cache_expiring_ms):
            if not await self.sync():
                return None
        return self.position.predict(at_us) if at_us else self.position

    def predict_position(self, at: Optional[int] = None) -> Optional[Position]:
        return self.position.predict(at=at) if self.position else None

    @BaseModule.use_session(
        terminal_type=rpc.ShipI,
        return_on_unreachable=None,
        return_on_cancel=None)
    async def get_state(self,
                        cache_expiring_ms: int = 50,
                        session: Optional[rpc.ShipI] = None) \
            -> Optional[rpc.ShipState]:
        """Return current ship's state"""
        assert session is not None
        if self.state and not self.state.expired(cache_expiring_ms):
            return self.state
        self.state = await session.get_state()
        return self.state

    def start_monitoring(self, interval_ms: int = 50):
        self.__monitoring_task = asyncio.get_running_loop().create_task(
            self.__monitoring(interval_ms))

    def stop_monitoring(self):
        if self.__monitoring_task:
            self.__monitoring_task.cancel()
            self.__monitoring_task = None

    def __on_update(self, state: rpc.ShipState):
        self.state = state
        if state.position:
            self.position = state.position

    @BaseModule.use_session(
        terminal_type=rpc.ShipI,
        exclusive=True,
        return_on_unreachable=None,
        return_on_cancel=None)
    async def __monitoring(
            self,
            interval_ms: int,
            session: Optional[rpc.ShipI] = None):
        assert session is not None
        ack = await session.monitor(interval_ms)
        if ack is None:
            return
        timeout = 5 * interval_ms / 1000
        while True:
            state = await session.wait_state(timeout)
            if not state:
                return
            self.__on_update(state)

    @staticmethod
    def get_ship_by_name(
            commutator: "Commutator",
            name: str) -> Optional["Ship"]:
        for module_type, name2ship in commutator.modules.items():
            if module_type.startswith(ModuleType.SHIP.value):
                try:
                    ship = name2ship[name]
                    assert isinstance(ship, Ship)
                    return ship
                except KeyError:
                    continue
        return None
