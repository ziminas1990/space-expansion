from typing import Tuple, Optional, Awaitable, Callable
import asyncio

from base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world

from server.configurator.modules import (
    default_ships,
    shipyard_blueprints,
    ShipType,
    ResourceContainerState
)
from server.configurator.configuration import Configuration
from server.configurator.general import General, ApplicationMode
from server.configurator.resources import ResourcesList

from expansion.modules import Ship, Shipyard, ResourceContainer
from expansion.types import ResourceType, ResourceItem


class ProgressTracker():
    def __init__(self, proceed: Callable[[], Awaitable[None]]):
        self.proceed: Callable[[], Awaitable[None]] = proceed
        self.status: Optional[Shipyard.Status] = None
        self.progress = 0.0
        self.report_event = asyncio.Event()

    def on_progress(self, status, progress):
        self.status = status
        self.progress = progress
        self.report_event.set()

    async def next_report(self, timeout: float = 3) \
            -> Tuple[Optional[Shipyard.Status], Optional[float]]:
        async def _wait_report_event():
            while not self.report_event.is_set():
                await self.proceed()

        try:
            await asyncio.wait_for(_wait_report_event(), timeout)
        except asyncio.TimeoutError:
            return None, None
        self.report_event.clear()
        return self.status, self.progress

    async def wait_report(
            self,
            expexcted_status: Shipyard.Status,
            timeout: float = 3) \
            -> Optional[float]:
        async def impl():
            status, progress = await self.next_report()
            while status != expexcted_status:
                status, progress = await self.next_report()
                if status is None:
                    return None
            return progress

        try:
            progress = await asyncio.wait_for(impl(), timeout)
            return progress
        except asyncio.TimeoutError:
            return None

    async def wait_complete_report(self, timeout: float = 3) -> Optional[float]:
        return await self.wait_report(Shipyard.Status.BUILD_COMPLETE, timeout)

    async def wait_frozen_report(self, timeout: float = 3) -> Optional[float]:
        return await self.wait_report(Shipyard.Status.BUILD_FROZEN, timeout)

    async def wait_progress_report(self, timeout: float = 3) -> Optional[float]:
        return await self.wait_report(Shipyard.Status.BUILD_IN_PROGRESS, timeout)


class TestCase(BaseTestFixture):

    def __init__(self, *args, **kwargs):
        super(TestCase, self).__init__(*args, **kwargs)

        self.configuration = Configuration(
            general=General(total_threads=1,
                            login_udp_port=7456,
                            initial_state=ApplicationMode.e_FREEZE,
                            ports_pool=(12000, 12100)),
            blueprints=blueprints.DefaultBlueprints(),
            players={
                'player': world.Player(
                    login="player",
                    password="awesome",
                    ships=[
                        default_ships.make_station(
                            name="SweetHome",
                            position=world.Position(
                                x=0, y=0, velocity=world.Vector(0, 0)),
                            warehouse=ResourceContainerState({
                                ResourceType.e_METALS: 200000,
                                ResourceType.e_SILICATES: 40000,
                            }),
                        ),
                    ]
                )
            },
            world=world.World(),
        )

    def get_configuration(self) -> Configuration:
        return self.configuration

    @BaseTestFixture.run_as_sync
    async def test_get_specification(self):
        await self.system_clock_fast_forward(speed_multiplier=20)

        commutator, error = await self.login('player', "127.0.0.1")
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        station = Ship.get_ship_by_name(commutator, "SweetHome")
        self.assertIsNotNone(station)

        shipyard_medium = Shipyard.find_by_name(station, "shipyard-medium")
        shipyard_large = Shipyard.find_by_name(station, "shipyard-large")
        self.assertIsNotNone(shipyard_medium)
        self.assertIsNotNone(shipyard_large)

        shipyard_blueprint = shipyard_blueprints["medium"]
        status, spec = await shipyard_medium.get_specification()
        self.assertEqual(status, Shipyard.Status.SUCCESS)
        self.assertIsNotNone(spec)
        self.assertEqual(shipyard_blueprint.productivity, spec.labor_per_sec)

        shipyard_blueprint = shipyard_blueprints["large"]
        status, spec = await shipyard_large.get_specification()
        self.assertEqual(status, Shipyard.Status.SUCCESS)
        self.assertIsNotNone(spec)
        self.assertEqual(shipyard_blueprint.productivity, spec.labor_per_sec)

    @BaseTestFixture.run_as_sync
    async def test_building(self):
        await self.system_clock_fast_forward(speed_multiplier=20)

        miner_blueprint = default_ships.ships_blueprints[ShipType.MINER]

        commutator, error = await self.login('player', "127.0.0.1")
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        station = Ship.get_ship_by_name(commutator, "SweetHome")
        self.assertIsNotNone(station)

        warehouse = ResourceContainer.get_by_name(station, "warehouse")
        self.assertIsNotNone(warehouse)

        shipyard_container = ResourceContainer.get_by_name(station, "shipyard-container")
        self.assertIsNotNone(shipyard_container)

        shipyard_large = Shipyard.find_by_name(station, "shipyard-large")
        self.assertIsNotNone(shipyard_large)
        self.assertEqual(Shipyard.Status.SUCCESS,
                         await shipyard_large.bind_to_cargo("shipyard-container"))

        # Open port on shipyard's container
        access_key = 1234
        status, port = await shipyard_container.open_port(access_key)
        self.assertEqual(ResourceContainer.Status.SUCCESS, status)

        ship_expenses = self.configuration.blueprints.ship_expenses(
            miner_blueprint
        )

        content = await warehouse.get_content()
        self.assertIsNotNone(content)
        self.assertTrue(ResourcesList(content.resources).contains(ship_expenses))

        # Building a ship
        async def proceed():
            await self.system_clock_proceed(450, 1, 50000)

        build_tracker = ProgressTracker(proceed=proceed)
        task = asyncio.create_task(shipyard_large.build_ship(
            blueprint=str(miner_blueprint.id),
            ship_name="SCV",
            progress_cb=build_tracker.on_progress))

        progress = await build_tracker.wait_frozen_report()

        # Since there are no resources in shipyard's container,
        # BUILD_FROZEN indication is expected
        self.assertIsNotNone(progress)
        self.assertAlmostEqual(0, progress)

        delta = 0.001
        for i in range(1, 11):
            # Move to shipyard's container 10% of all required resources
            await self.system_clock_fast_forward(100, 10000)
            for resource, amount in ship_expenses.resources.items():
                if resource == ResourceType.e_LABOR:
                    continue
                status = await warehouse.transfer(
                    port,
                    access_key,
                    ResourceItem(resource, amount / 10))
                self.assertEqual(ResourceContainer.Status.SUCCESS, status, f"Iteration {i}")
            await self.system_clock_play()
            # Waiting for progress indications
            while progress + delta < 0.1 * i:
                progress = await build_tracker.wait_progress_report()
                self.assertIsNotNone(progress, f"Iteration {i}")
            self.assertAlmostEqual(progress, 0.1 * i, delta=delta)
            if i < 10:
                # Shipyard's container is empty
                for j in range(10):
                    progress = await build_tracker.wait_frozen_report()
                    self.assertIsNotNone(progress, f"Iteration {i}.{j}")
                    self.assertAlmostEqual(progress, 0.1 * i, delta=delta)

        # Waiting for 'build complete' indication
        progress = await build_tracker.wait_complete_report()
        self.assertAlmostEqual(1, progress)

        status, ship_name, slot_id = await task
        self.assertEqual(Shipyard.Status.SUCCESS, status)
