import types
from typing import Optional
import asyncio

from expansion import modules
from expansion.modules import Commutator, SystemClock, ModuleType, Shipyard, ResourceContainer
from expansion.types import TimePoint, Position, Vector
import expansion.interfaces.rpc as rpc
import logging

from ship import Ship
from world import World
from player import Player

from tasks.mining.random_mining import RandomMining


class TacticalCore:
    def __init__(self, root_commutator: Commutator):
        self.player = Player()
        self.world = World()
        self.root_commutator = root_commutator
        self.system_clock: Optional[SystemClock] = None
        self.time = TimePoint(0)
        self.log: logging.Logger = logging.getLogger("TacticalCore")

        self.commutator_monitoring_task: Optional[asyncio.Task] = None
        self.time_monitoring_task: Optional[asyncio.Task] = None
        self.build_miners_task: Optional[asyncio.Task] = None
        self.random_mining_task: Optional[RandomMining] = None

    async def initialize(self) -> bool:
        # Initializing root commutator and system clock
        if not await self.root_commutator.init():
            self.log.error("Failed to init root commutator!")
            return False

        self.system_clock = modules.get_system_clock(self.root_commutator)
        if self.system_clock is None:
            self.log.error("SystemClock not found!")
            return False

        self.time_monitoring_task = asyncio.create_task(self.monitor_time())

        # Getting all available ships:
        remote_ships = modules.Ship.get_all_ships(self.root_commutator)
        for remote_ship in remote_ships:
            ship = Ship(
                remote=remote_ship,
                the_world=self.world,
                system_clock=self.system_clock)
            ship.start_self_monitoring()
            ship.start_passive_scanning()
            self.player.ships[ship.name] = ship

        return True

    def stop(self):
        self.root_commutator.disconnect()

    def move_ship(self, ship_name:str, x: float, y: float):
        try:
            ship = self.player.ships[ship_name]
            ship.navigator.move_to_async(Position(x, y, Vector(0, 0)))
        except KeyError:
            self.log.warning(f"Can't move ship '{ship_name}': no such ship!")

    async def build_more_miners(self):
        ships = self.player.get_ships_by_equipment(
            [ModuleType.SHIPYARD, ModuleType.RESOURCE_CONTAINER]
        )
        if not ships:
            self.log.error("Can't get Warehouse!")
            return
        warehouse = ships[0]

        shipyard: Optional[Shipyard] = await Shipyard.find_most_productive(
            warehouse.commutator())
        if not shipyard:
            self.log.error("Can't get shipyard!")
            return

        cargo: Optional[ResourceContainer] = await ResourceContainer.find_most_voluminous(
            warehouse.commutator()
        )
        if not cargo:
            self.log.error("Can't get resource container!")
            return

        async def monitor_cargo():
            async for content in cargo.monitor():
                self.log.info(f"Shipyard cargo: {content}")

        asyncio.create_task(monitor_cargo())
        await shipyard.bind_to_cargo(cargo.name)

        next_id = 10
        ship_type = "Ship/Civilian-Miner"
        while True:
            await shipyard.build_ship(
                ship_type, f"Miner-{next_id}",
                lambda s, p: self.log.info(f"Shipyard: {s} {p}"))
            next_id += 1

    async def monitor_commutator(self):
        while True:
            async for update in self.root_commutator.monitoring():
                if update is not None:
                    assert isinstance(update, rpc.CommutatorUpdate)
                    if update.module_attached:
                        if update.module_attached.type.startswith("Ship/"):
                            self.__on_ship_spawned(update.module_attached)

    def __on_ship_spawned(self, ship_info: rpc.ModuleInfo):
        remote_ship = modules.Ship.get_ship_by_name(
            commutator=self.root_commutator,
            name=ship_info.name
        )
        if remote_ship is None:
            return
        ship = Ship(remote=remote_ship,
                    the_world=self.world,
                    system_clock=self.system_clock)
        assert ship.name not in self.player.ships
        self.player.ships.update({ship.name: ship})

        ship.start_self_monitoring()
        ship.start_passive_scanning()
        if RandomMining.can_use_ship(ship):
            self.random_mining_task.add_ship(ship)

    async def run(self):
        # Initialize RandomMiner
        ships = self.player.get_ships_by_equipment(
            [ModuleType.SHIPYARD, ModuleType.RESOURCE_CONTAINER]
        )
        if not ships:
            self.log.error("Can't get Warehouse!")
            return
        warehouse = ships[0]
        self.log.info(f"Using '{warehouse.name}' as warehouse")

        self.random_mining_task = RandomMining(
            "RandomMining", self, warehouse, self.system_clock
        )
        mining_ships = self.player.get_ships_by_equipment(
            [ModuleType.RESOURCE_CONTAINER,
             ModuleType.ASTEROID_MINER,
             ModuleType.ENGINE]
        )
        for miner in mining_ships:
            self.log.info(f"Using '{miner.name}' as mining ship")
            self.random_mining_task.add_ship(miner)
        self.random_mining_task.run_async()

        self.build_miners_task = asyncio.create_task(self.build_more_miners())
        self.commutator_monitoring_task = asyncio.create_task(
            self.monitor_commutator()
        )

        """Main tactical core loop"""
        while True:
            # Nothing to do here
            await asyncio.sleep(0.1)

    async def monitor_time(self):
        while True:
            async for _ in self.system_clock.monitor(40):
                self.time = self.system_clock.time_point()
            # Something went wrong. Try again in 250 milliseconds
            await asyncio.sleep(0.25)
