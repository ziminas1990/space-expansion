from typing import Optional
import asyncio

from expansion import modules
from expansion.modules import Commutator, SystemClock, ModuleType, Shipyard, ResourceContainer
from expansion.types import TimePoint, Position, Vector
import logging

from assistants import AsteroidTracker, ScanningParams
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

        self.asteroids_tracker: Optional[AsteroidTracker] = None
        self.random_mining: Optional[RandomMining] = None

    async def initialize(self) -> bool:
        # Initializing root commutator and system clock
        if not await self.root_commutator.init():
            self.log.error("Failed to init root commutator!")
            return False

        self.system_clock = modules.get_system_clock(self.root_commutator)
        if self.system_clock is None:
            self.log.error("SystemClock not found!")
            return False
        self.time = await self.system_clock.time()
        self.system_clock.subscribe(self._on_time_cb)

        # Getting all available ships:
        remote_ships = modules.get_all_ships(self.root_commutator)
        for remote_ship in remote_ships:
            if not remote_ship.start_monitoring():
                self.log.warning(f"Failed to start monitoring ship {remote_ship.name}")
            self.player.ships[remote_ship.name] = Ship(remote_ship, self.system_clock)

        # Assistants
        self.asteroids_tracker: AsteroidTracker = \
            AsteroidTracker(
                root_commutator=self.root_commutator,
                world=self.world,
                system_clock=self.system_clock)

        # Starting auto-scanning
        self.asteroids_tracker.run_auto_scanning([
            ScanningParams(10, 5),
            ScanningParams(100, 10),
            ScanningParams(500, 15),
        ])
        return True

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
            status, name, port =\
                await shipyard.build_ship(
                    ship_type, f"Miner-{next_id}",
                    lambda s, p: self.log.info(f"Shipyard: {s} {p}"))
            next_id += 1
            success = self.root_commutator.add_module(ship_type, name, port)
            if not success:
                self.log.error("Failed to register new ship")
                continue
            remote_ship = modules.Ship.get_ship_by_name(self.root_commutator, name)
            miner = Ship(remote_ship, self.system_clock)
            assert name not in self.player.ships
            success = await miner.remote.init()
            if not success:
                self.log.error(f"Failed to init miner {name}")
                continue
            remote_ship.start_monitoring()
            self.player.ships.update({name: miner})
            self.random_mining.add_ship(miner)

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

        self.random_mining = RandomMining(
            "RandomMining", self, warehouse, self.system_clock
        )
        mining_ships = self.player.get_ships_by_equipment(
            [ModuleType.RESOURCE_CONTAINER,
             ModuleType.ASTEROID_MINER,
             ModuleType.ENGINE]
        )
        for miner in mining_ships:
            self.log.info(f"Using '{miner.name}' as mining ship")
            self.random_mining.add_ship(miner)
        self.random_mining.run_async()

        shipyard_task = asyncio.create_task(self.build_more_miners())

        """Main tactical core loop"""
        while True:
            # Nothing to do here
            await asyncio.sleep(0.1)

    def _on_time_cb(self, time: TimePoint):
        self.time = time
