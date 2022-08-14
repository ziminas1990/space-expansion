from typing import List, Dict
from base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world

from server.configurator.modules import default_ships, ShipType
from server.configurator.configuration import Configuration
from server.configurator.general import General, ApplicationMode

from expansion import modules, types
from randomizer import Randomizer


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
            world=world.World(),
        )

    def get_configuration(self) -> Configuration:
        return self.configuration

    @staticmethod
    async def scanning(scaner: modules.PassiveScanner,
                       clock: modules.SystemClock,
                       scanning_time_ms: int) \
            -> Dict[int, types.PhysicalObject]:
        start_at_usec = await clock.time()
        end_at_usec = start_at_usec + scanning_time_ms * 1000

        scanning_result: Dict[int, types.PhysicalObject] = {}
        async for detected_object in scaner.scan():
            scanning_result.update({detected_object.object_id: detected_object})
            now_usec = await clock.time()
            if now_usec > end_at_usec:
                return scanning_result

    @BaseTestFixture.run_as_sync
    async def test_get_specification(self):
        await self.system_clock_fast_forward(speed_multiplier=20)

        commutator, error = await self.login('oreman', "127.0.0.1")
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        miner_1 = modules.get_ship(commutator, ShipType.MINER.value, "miner-1")
        self.assertIsNotNone(miner_1)

        scanner = modules.PassiveScanner.get_by_name(miner_1, "perceiver")
        self.assertIsNotNone(scanner)

        spec = await scanner.get_specification()
        self.assertIsNotNone(spec)

    @BaseTestFixture.run_as_sync
    async def test_simple_scan(self):
        randomizer = Randomizer(seed=3284)
        await self.system_clock_fast_forward(speed_multiplier=20)

        commutator, error = await self.login('oreman', "127.0.0.1")
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)
        clock = modules.get_system_clock(commutator)
        self.assertIsNotNone(clock)

        miner_1 = modules.get_ship(commutator, ShipType.MINER.value, "miner-1")
        self.assertIsNotNone(miner_1)
        scanner = modules.PassiveScanner.get_by_name(miner_1, "perceiver")
        self.assertIsNotNone(scanner)
        spec = await scanner.get_specification()
        self.assertIsNotNone(spec)

        ship_position = await miner_1.get_position()

        spawned_asteroids: List[types.PhysicalObject] = []
        for i in range(1000):
            status, asteroid =\
                await self.administrator.get_spawner().spawn_asteroid(
                    position=randomizer.random_position(
                        center=ship_position,
                        radius=2 * spec.scanning_radius_km * 1000
                    ),
                    composition=types.make_resources(ice=100, metals=32),
                    radius=randomizer.random_value(5, 20)
            )
            spawned_asteroids.append(asteroid)

        # Take 10 seconds to scan
        scanned_objects = await TestCase.scanning(scanner, clock, 10000)

        for candidate in spawned_asteroids:
            distance = ship_position.distance_to(candidate.position)
            if distance <= spec.scanning_radius_km * 1000:
                self.assertIn(candidate.object_id, scanned_objects)
