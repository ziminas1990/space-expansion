from typing import Optional, Tuple, Callable, Awaitable
import time

from expansion.interfaces.rpc import ShipI, ShipState
from expansion.types import Position
from .base_module import BaseModule
from .commutator import Commutator, ModulesFactory


class Ship(Commutator, BaseModule):
    """This class represent a wrapper for the remote ship. Is supports
    caching and prediction ship's position"""

    @staticmethod
    def factory(ship_type: str,
                ship_name: str,
                open_tunnel_f: Callable[[], Awaitable[ShipI]]):
        Ship(ship_type, ship_name, open_tunnel_f)

    def __init__(self,
                 ship_type: str,
                 ship_name: str,
                 modules_factory: ModulesFactory,
                 connection_factory: Callable[[], Awaitable[ShipI]]):
        super().__init__(
            connection_factory=connection_factory,
            modules_factory=modules_factory,
            name=ship_name)
        self.type = ship_type
        self.name = ship_name

        self.position: Optional[Position] = None
        self.state: Optional[ShipState] = None

    async def init(self):
        await super().init()

    async def sync(self, timeout: float = 0.5) -> bool:
        """Update ship's information."""
        async with self._lock_channel() as channel:
            assert isinstance(channel, ShipI)  # for type hinting
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
        Otherwise a predicted position will be returned.
        If updating position is required (cache has expired), the request will be
        sent to the server and this call will block the thread until the response is
        received or the specified 'timeout' occurs.
        """
        if self.position is None:
            if not await self.sync(timeout=timeout):
                return None
            return self.position.predict(at_us)

        if self.position.expired(cache_expiring_ms):
            if not await self.sync():
                return None
        return self.position.predict(at_us)

    async def get_state(self, cache_expiring_ms: int = 50) -> Optional[ShipState]:
        """Return current ship's state"""
        if self.state and not self.state.expired(cache_expiring_ms):
            return self.state

        async with self._lock_channel() as channel:
            assert isinstance(channel, ShipI)  # for type hinting
            self.state = await channel.get_state()
            return self.state
