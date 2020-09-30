from typing import Dict
import  asyncio

from tests.base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world

from server.configurator.modules import default_ships, asteroid_miner_blueprints
from server.configurator import Configuration, General, ApplicationMode

import expansion.procedures as procedures
from expansion.interfaces.public import AsteroidMiner, CelestialScanner
from expansion.types import ResourceType, ResourceItem


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
                'oreman': world.Player(
                    login="oreman",
                    password="thinbones",
                    ships=[
                        default_ships.make_miner(
                            name="miner-1",
                            position=world.Position(
                                x=0, y=0, velocity=world.Vector(0, 0))),
                    ]
                )
            },
            world=world.World(
                asteroids=world.Asteroids(asteroids=[
                    world.Asteroid(position=world.Position(x=100, y=100),
                                   radius=10,
                                   composition={world.ResourceType.e_ICE: 50,
                                                world.ResourceType.e_SILICATES: 20,
                                                world.ResourceType.e_METALS: 20}),
                    world.Asteroid(position=world.Position(x=5000, y=5000),
                                   radius=20,
                                   composition={world.ResourceType.e_ICE: 10,
                                                world.ResourceType.e_SILICATES: 10,
                                                world.ResourceType.e_METALS: 40})
                ])),
        )

    def get_configuration(self) -> Configuration:
        return self.configuration

    @BaseTestFixture.run_as_sync
    async def test_get_specification(self):
        await self.system_clock_fast_forward(speed_multiplier=20)

        commutator, error = await self.login('oreman')
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        miner_ship = await procedures.connect_to_ship("Miner", "miner-1", commutator)
        self.assertIsNotNone(miner_ship)

        miner = await procedures.connect_to_asteroid_miner("miner", miner_ship)
        self.assertIsNotNone(miner)

        miner_blueprint = asteroid_miner_blueprints["basic"]

        status, spec = await miner.get_specification()
        self.assertEqual(AsteroidMiner.Status.SUCCESS, status)
        self.assertIsNotNone(spec)
        self.assertEqual(miner_blueprint.max_distance, spec.max_distance)
        self.assertEqual(miner_blueprint.cycle_time_ms, spec.cycle_time_ms)
        self.assertEqual(miner_blueprint.yield_per_cycle, spec.yield_per_cycle)

    @BaseTestFixture.run_as_sync
    async def test_binding_to_cargo(self):
        await self.system_clock_fast_forward(speed_multiplier=20)

        commutator, error = await self.login('oreman')
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        miner_ship = await procedures.connect_to_ship("Miner", "miner-1", commutator)
        self.assertIsNotNone(miner_ship)

        miner = await procedures.connect_to_asteroid_miner("miner", miner_ship)
        self.assertIsNotNone(miner)

        status = await miner.bind_to_cargo("invalid cargo")
        self.assertEqual(AsteroidMiner.Status.NOT_BOUND_TO_CARGO, status)
        self.assertIsNone(miner.cargo_name)

        status = await miner.bind_to_cargo("cargo")
        self.assertEqual(AsteroidMiner.Status.SUCCESS, status)
        self.assertEqual("cargo", miner.cargo_name)

    @BaseTestFixture.run_as_sync
    async def test_start_mining(self):
        await self.system_clock_fast_forward(speed_multiplier=50)

        commutator, error = await self.login('oreman')
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        miner_ship = await procedures.connect_to_ship("Miner", "miner-1", commutator)
        self.assertIsNotNone(miner_ship)

        miner = await procedures.connect_to_asteroid_miner("miner", miner_ship)
        self.assertIsNotNone(miner)

        status = await miner.bind_to_cargo("cargo")
        self.assertEqual(AsteroidMiner.Status.SUCCESS, status)
        self.assertEqual("cargo", miner.cargo_name)

        def progress_cb(status: AsteroidMiner.Status, item: ResourceItem) -> bool:
            return False  # Stop mining immediately

        # Trying to mine non existing asteroid
        status = await miner.start_mining(asteroid_id=100500,
                                          resource=ResourceType.e_METALS,
                                          progress_cb=progress_cb)
        self.assertEqual(AsteroidMiner.Status.ASTEROID_DOESNT_EXIST, status)

        # Looking for the asteroid, that should be away
        scanner: CelestialScanner = \
            await procedures.connect_to_celestial_scanner("scanner", miner_ship)
        self.assertIsNotNone(scanner)

        result, error = await scanner.scan(scanning_radius_km=10, minimal_radius_m=15)
        self.assertIsNone(error)
        self.assertIsNotNone(result)
        self.assertEqual(1, len(result))
        asteroid = result[0]

        # Mining an asteroid
        status = await miner.start_mining(asteroid_id=asteroid.object_id,
                                          resource=ResourceType.e_METALS,
                                          progress_cb=progress_cb)
        self.assertEqual(AsteroidMiner.Status.ASTEROID_TOO_FAR, status)

        # Looking for the asteroid, that should be nearby
        scanner: CelestialScanner =\
            await procedures.connect_to_celestial_scanner("scanner", miner_ship)
        self.assertIsNotNone(scanner)

        result, error = await scanner.scan(scanning_radius_km=1, minimal_radius_m=5)
        self.assertIsNone(error)
        self.assertIsNotNone(result)
        self.assertEqual(1, len(result))
        asteroid = result[0]

        # Mining an asteroid
        status = await miner.start_mining(asteroid_id=asteroid.object_id,
                                          resource=ResourceType.e_METALS,
                                          progress_cb=progress_cb)
        self.assertEqual(AsteroidMiner.Status.SUCCESS, status)

    @BaseTestFixture.run_as_sync
    async def test_mining(self):
        await self.system_clock_fast_forward(speed_multiplier=50)

        commutator, error = await self.login('oreman')
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        miner_ship = await procedures.connect_to_ship("Miner", "miner-1", commutator)
        self.assertIsNotNone(miner_ship)

        miner = await procedures.connect_to_asteroid_miner("miner", miner_ship)
        self.assertIsNotNone(miner)

        status = await miner.bind_to_cargo("cargo")
        self.assertEqual(AsteroidMiner.Status.SUCCESS, status)
        self.assertEqual("cargo", miner.cargo_name)

        # Looking for the asteroid, that should be nearby
        scanner: CelestialScanner = \
            await procedures.connect_to_celestial_scanner("scanner", miner_ship)
        self.assertIsNotNone(scanner)

        result, error = await scanner.scan(scanning_radius_km=1, minimal_radius_m=5)
        self.assertIsNone(error)
        self.assertIsNotNone(result)
        self.assertEqual(1, len(result))
        asteroid = result[0]

        # Mining until we have 1000 of metals
        collected_resources: Dict[ResourceType, float] = {
            ResourceType.e_METALS: 0
        }
        last_received_status: AsteroidMiner.Status = AsteroidMiner.Status.SUCCESS

        def progress_cb(status: AsteroidMiner.Status, item: ResourceItem) -> bool:
            nonlocal last_received_status
            last_received_status = status
            collected_resources[item.resource_type] += item.amount
            return collected_resources[ResourceType.e_METALS] < 1000

        # Mining an asteroid
        await self.system_clock_fast_forward(speed_multiplier=500)
        status = await miner.start_mining(asteroid_id=asteroid.object_id,
                                          resource=ResourceType.e_METALS,
                                          progress_cb=progress_cb)
        self.assertEqual(AsteroidMiner.Status.SUCCESS, status)
        await self.system_clock_fast_forward(speed_multiplier=50)

        # Check the cargo content
        cargo = await procedures.connect_to_resource_container("cargo", miner_ship)
        self.assertIsNotNone(cargo)
        content = await cargo.get_content()
        self.assertIsNotNone(content)
        self.assertLessEqual(1000, content.resources[ResourceType.e_METALS])

    @BaseTestFixture.run_as_sync
    async def test_stop_mining(self):
        await self.system_clock_fast_forward(speed_multiplier=50)

        commutator, error = await self.login('oreman')
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        miner_ship = await procedures.connect_to_ship("Miner", "miner-1", commutator)
        self.assertIsNotNone(miner_ship)

        miner = await procedures.connect_to_asteroid_miner("miner", miner_ship)
        self.assertIsNotNone(miner)
        status, miner_spec = await miner.get_specification()
        self.assertEqual(status, AsteroidMiner.Status.SUCCESS)
        self.assertIsNotNone(miner_spec)

        status = await miner.bind_to_cargo("cargo")
        self.assertEqual(AsteroidMiner.Status.SUCCESS, status)
        self.assertEqual("cargo", miner.cargo_name)

        status = await miner.stop_mining()
        self.assertEqual(AsteroidMiner.Status.MINER_IS_IDLE, status)

        # Looking for the asteroid, that should be nearby
        scanner: CelestialScanner = \
            await procedures.connect_to_celestial_scanner("scanner", miner_ship)
        self.assertIsNotNone(scanner)

        result, error = await scanner.scan(scanning_radius_km=1, minimal_radius_m=5)
        self.assertIsNone(error)
        self.assertIsNotNone(result)
        self.assertEqual(1, len(result))
        asteroid = result[0]

        # Mining an asteroid
        def progress_cb(status: AsteroidMiner.Status, item: ResourceItem) -> bool:
            return True

        mining_task = asyncio.get_running_loop().create_task(
            miner.start_mining(asteroid_id=asteroid.object_id,
                               resource=ResourceType.e_METALS,
                               progress_cb=progress_cb)
        )
        # waiting for some cycles
        ok, _ = await self.system_clock_proceed(proceed_ms=miner_spec.cycle_time_ms * 5.5,
                                                timeout_s=5)
        self.assertTrue(ok)
        self.assertFalse(mining_task.done())

        # Stopping mining process
        await self.system_clock_fast_forward(50)
        status = await miner.stop_mining()
        self.assertEqual(AsteroidMiner.Status.SUCCESS, status)

        status = await asyncio.wait_for(mining_task, 0.1)
        self.assertEqual(AsteroidMiner.Status.INTERRUPTED_BY_USER, status)

    @BaseTestFixture.run_as_sync
    async def test_cargo_is_full(self):
        await self.system_clock_fast_forward(speed_multiplier=50)

        commutator, error = await self.login('oreman')
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        miner_ship = await procedures.connect_to_ship("Miner", "miner-1", commutator)
        self.assertIsNotNone(miner_ship)

        miner = await procedures.connect_to_asteroid_miner("miner", miner_ship)
        self.assertIsNotNone(miner)
        status, miner_spec = await miner.get_specification()
        self.assertEqual(status, AsteroidMiner.Status.SUCCESS)
        self.assertIsNotNone(miner_spec)

        status = await miner.bind_to_cargo("tiny_cargo")
        self.assertEqual(AsteroidMiner.Status.SUCCESS, status)
        self.assertEqual("tiny_cargo", miner.cargo_name)

        # Looking for the asteroid, that should be nearby
        scanner: CelestialScanner = \
            await procedures.connect_to_celestial_scanner("scanner", miner_ship)
        self.assertIsNotNone(scanner)

        result, error = await scanner.scan(scanning_radius_km=1, minimal_radius_m=5)
        self.assertIsNone(error)
        self.assertIsNotNone(result)
        self.assertEqual(1, len(result))
        asteroid = result[0]

        # Mining an asteroid
        def progress_cb(status: AsteroidMiner.Status, item: ResourceItem) -> bool:
            return True

        # This will take a long time
        await self.system_clock_fast_forward(1000)
        status = await miner.start_mining(asteroid_id=asteroid.object_id,
                                          resource=ResourceType.e_ICE,
                                          progress_cb=progress_cb)
        self.assertEqual(AsteroidMiner.Status.NO_SPACE_AVAILABLE, status)

        # Check that cargo is full of ice
        cargo = await procedures.connect_to_resource_container("tiny_cargo", miner_ship)
        self.assertIsNotNone(cargo)
        content = await cargo.get_content()
        self.assertIsNotNone(content)
        self.assertAlmostEqual(content.volume, content.used)
        self.assertAlmostEqual(
            ResourceType.calculate_amount(ResourceType.e_ICE, content.volume),
            content.resources[ResourceType.e_ICE])