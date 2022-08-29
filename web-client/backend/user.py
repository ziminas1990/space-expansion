from typing import Dict, Optional, List
import asyncio
from expansion import modules, types, procedures
from expansion.interfaces import rpc
import secrets


class User:
    __users_cache: Dict[str, "User"] = {}

    def __init__(self, user_id: str):
        self.user_id: str = user_id
        self.commutator: Optional[modules.Commutator] = None
        self.system_clock: Optional[modules.SystemClock] = None
        self.ships: List[modules.Ship] = []
        self.asteroids: Dict[int, types.PhysicalObject] = {}
        self.token: str = secrets.token_urlsafe(8)

        # Hack: lump all async tasks to one pile
        self._async_tasks: List[asyncio.Task] = []
        self.now: int = 0

    def connected(self) -> bool:
        return self.commutator is not None

    def is_authenticated(self) -> bool:
        return self.connected()

    def is_active(self) -> bool:
        return True

    def is_anonymous(self) -> bool:
        return self.user_id == "anonymous"

    def get_id(self) -> str:
        return self.user_id

    async def login(self, ip: str, login: str, password: str) -> Optional[str]:
        self.commutator, problem = await procedures.login(
            server_ip=ip,
            login_port=6842,
            login=login,
            password=password)
        if problem:
            return problem
        success = await self.commutator.init()
        return None if success else "Failed to initialize commutator"

    async def initialize(self) -> Optional[str]:
        self.system_clock = modules.SystemClock.find(self.commutator)
        if self.system_clock is None:
            return "Failed to instantiate system clock"
        self._async_tasks.append(
            asyncio.create_task(self._monitor_time()))
        self._async_tasks.append(
            asyncio.create_task(self._monitor_commutator()))

        # Export all ships and start monitoring for them
        for ship in modules.Ship.get_all_ships(self.commutator):
            await self.on_new_ship(ship)
        return None

    async def on_new_ship(self, ship: modules.Ship):
        self._async_tasks.append(ship.create_self_monitoring_task())
        self.ships.append(ship)
        # if ship has a passive scanner, attach to it
        scanner = await modules.PassiveScanner.get_most_ranged(ship)
        if scanner:
            self._async_tasks.append(
                asyncio.create_task(self._scanning_task(scanner)))

    async def _scanning_task(self, passive_scaner: modules.PassiveScanner):
        while True:
            async for item in passive_scaner.scan():
                # Assert for type hinting
                assert isinstance(item, types.PhysicalObject)
                if item.object_type == types.ObjectType.ASTEROID:
                    self.asteroids.update({item.object_id: item})
            # Something went wrong. Try to start scanning in a second
            await asyncio.sleep(1)

    async def _monitor_time(self):
        while True:
            async for time in self.system_clock.monitor(interval_ms=25):
                self.now = time
            # Something went wrong. Try one more time in 0.25 seconds
            await asyncio.sleep(0.25)

    async def _monitor_commutator(self):
        while True:
            async for update in self.commutator.monitoring():
                if update is None:
                    continue
                assert isinstance(update, rpc.CommutatorUpdate)
                if update.module_attached:
                    if update.module_attached.type.startswith("Ship/"):
                        ship = modules.Ship.get_ship_by_name(
                            commutator=self.commutator,
                            name=update.module_attached.name
                        )
                        if ship:
                            await self.on_new_ship(ship)
            # Something went wrong, try one more time in 0.25 seconds
            await asyncio.sleep(0.25)

    @staticmethod
    def access(user_id: str, token: str):
        try:
            user = User.__users_cache[user_id]
            return user if user.token == token else None
        except KeyError:
            return None

    @staticmethod
    def load(user_id: str):
        return User.__users_cache.setdefault(user_id, User(user_id))
