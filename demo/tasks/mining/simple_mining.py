import asyncio
import random
from typing import Optional, TYPE_CHECKING

from tasks.base_task import BaseTask
from expansion.modules import AsteroidMiner, ResourceContainer

if TYPE_CHECKING:
    from ship import Ship
    from tactical_core import TacticalCore
    from expansion.modules import (
        SystemClock,
    )
    from expansion.types import ResourceItem


class SimpleMining(BaseTask):

    def __init__(self,
                 name: str,
                 tactical_core: "TacticalCore",
                 ship: "Ship",
                 asteroid_id: int,
                 warehouse: "Ship",
                 system_clock: "SystemClock"):
        super().__init__(name, system_clock)
        self.tactical_core = tactical_core
        self.ship = ship
        self.asteroid_id = asteroid_id
        self.warehouse = warehouse

        # Auto detected modules:
        self._miner: Optional["AsteroidMiner"] = None
        self._container: Optional["ResourceContainer"] = None

    async def _impl(self, *argc, **argv) -> bool:
        if not await self._initial_checks():
            self.add_journal_record("Initial checks failed")
            return False
        self.add_journal_record("Initial checks done")

        if not await self._move_to_asteroid():
            self.add_journal_record("Moving to asteroid failed!")
            return False
        self.add_journal_record("Approached to the asteroid")

        if not await self._do_mining():
            self.add_journal_record("Mining failed!")
            return False
        self.add_journal_record("Mining finished")

        if not await self._return_to_warehouse():
            self.add_journal_record("Failed to return to warehouse")
            return False
        self.add_journal_record("Returned to the warehouse")

        if not await self._unload_the_product():
            self.add_journal_record("Failed to unload ore")
            return False
        self.add_journal_record("Ore is unloaded")

        self.add_journal_record("Finished successful!")
        return True

    async def _initial_checks(self) -> bool:
        self._miner = await AsteroidMiner.find_most_efficient(
            self.ship.commutator())
        if not self._miner:
            self.add_journal_record("Can't find appropriate miner!")
            return False
        self.add_journal_record(f"Use '{self._miner.name}' as miner")

        self._container = await ResourceContainer.find_most_free(
            self.ship.commutator())
        if not self._miner:
            self.add_journal_record("Can't find appropriate container!")
            return False
        self.add_journal_record(f"Use '{self._container.name}' as container")
        return True

    async def _move_to_asteroid(self) -> bool:
        asteroid = self.tactical_core.world.asteroids[self.asteroid_id]
        return await self.ship.navigator.move_to(asteroid.position.no_timestamp())

    async def _do_mining(self) -> bool:
        Status = AsteroidMiner.Status

        status: Status = await self._miner.bind_to_cargo(self._container.name)
        if not status.is_success():
            self.add_journal_record(
                f"Failed to attach '{self._miner.name}' miner to "
                f"'{self._container.name}' container: {status}")
            return False

        async def print_content():
            content = await self._container.get_content(timeout=1)
            self.add_journal_record(content.print_status())

        def mining_progress(status, resources) -> bool:
            if status.is_success():
                asyncio.create_task(print_content())
                return True
            else:
                self.add_journal_record(f"Mining stopped: {status}")
                return False

        status = await self._miner.start_mining(self.asteroid_id, mining_progress)
        self.add_journal_record(f"Mining complete with status {status}")
        return status.is_success() or status == Status.NO_SPACE_AVAILABLE

    async def _return_to_warehouse(self) -> bool:
        return await self.ship.navigator.move_to(await self.warehouse.get_position())

    async def _unload_the_product(self) -> bool:
        target_container = await ResourceContainer.find_most_free(
            self.warehouse.commutator(),
        )
        if not target_container:
            self.add_journal_record(
                f"Can't get resource container at "
                f"'{self.warehouse.name}' warehouse")
            return False
        self.add_journal_record(f"Unloading resources to '{target_container.name}' container")

        if target_container.opened_port is None:
            access_key = random.randint(0, 2**16)
            status, port = await target_container.open_port(
                access_key=access_key
            )
            if not status.is_success():
                self.add_journal_record(f"Failed to open port: {status}")
                return False

        port, access_key = target_container.opened_port

        content: Optional[ResourceContainer.Content] = await self._container.get_content()
        if content is None:
            self.add_journal_record(f"Failed to get content")
            return False

        def transfer_status(resource: "ResourceItem"):
            self.add_journal_record(f"{resource} transferred")

        for resource in content.as_list():
            status = await self._container.transfer(
                port=port,
                access_key=access_key,
                resource=resource,
                progress_cb=transfer_status)
            if not status.is_success():
                self.add_journal_record(f"Can't transfer {resource} to warehouse: {status}")
        return True
