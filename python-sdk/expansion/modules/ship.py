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
        self.__monitoring_session: Optional[rpc.Monitor] = None
        self.__monitoring_task: Optional[asyncio.Task] = None
        self.__monitoring_token: int = 0

    async def init(self) -> bool:
        return await super().init()

    async def sync(self, timeout: float = 0.5) -> bool:
        """Update ship's information."""
        async with self.rent_session(rpc.ShipI) as channel:
            position = await channel.navigation().get_position(timeout=timeout)
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

    async def get_state(self, cache_expiring_ms: int = 50) -> Optional[rpc.ShipState]:
        """Return current ship's state"""
        if self.state and not self.state.expired(cache_expiring_ms):
            return self.state

        async with self.rent_session(rpc.ShipI) as channel:
            self.state = await channel.get_state()
            return self.state

    async def start_monitoring(self, interval_ms: int = 50) -> bool:
        # If subscription session has not been opened yet we should open it
        if self.__monitoring_session is None:
            self.__monitoring_session = await self.open_session(rpc.Monitor)
            if not isinstance(self.__monitoring_session, rpc.Monitor):  # for type hinting
                return False
            status, self.__monitoring_token = await self.__monitoring_session.subscribe()
            if not status.is_ok():
                return False
            self.__monitoring_task = asyncio.get_running_loop().create_task(self.__monitoring())

        async with self.rent_session(rpc.ShipI) as session:
            if not isinstance(session, rpc.ShipI):
                return False
            ack = await session.monitor(interval_ms)
            return ack is not None

    async def stop_monitoring(self) -> bool:
        if self.__monitoring_session is None:
            return True
        status = await self.__monitoring_session.unsubscribe(self.__monitoring_token)
        return status.is_ok()

    def __on_update(self, state: rpc.ShipState):
        self.state = state
        if state.position:
            self.position = state.position

    async def __monitoring(self):
        await self.__monitoring_session.monitor(
            ship_state_cb=self.__on_update
        )

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
