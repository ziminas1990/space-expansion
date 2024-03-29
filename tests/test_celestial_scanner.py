from base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world

from server.configurator.modules import default_ships
from server.configurator.configuration import Configuration
from server.configurator.general import General, ApplicationMode

from expansion import modules


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
                    world.Asteroid(position=world.Position(x=10000, y=10000),
                                   radius=200,
                                   composition={world.ResourceType.e_ICE: 20,
                                                world.ResourceType.e_SILICATES: 20,
                                                world.ResourceType.e_METALS: 20})
                ])),
        )

    def get_configuration(self) -> Configuration:
        return self.configuration

    @BaseTestFixture.run_as_sync
    async def test_get_specification(self):
        await self.system_clock_fast_forward(speed_multiplier=20)

        connection, error = await self.login('oreman', "127.0.0.1")
        self.assertIsNotNone(connection)
        commutator = connection.commutator
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        miner = modules.get_ship(commutator, "Miner", "miner-1")
        self.assertIsNotNone(miner)

        scanner = modules.get_celestial_scanner(miner, "scanner")
        self.assertIsNotNone(scanner)

        spec = await scanner.get_specification()
        self.assertIsNotNone(spec)

        self.assertEqual(1000, spec.max_radius_km)
        self.assertEqual(10000, spec.processing_time_us)

    @BaseTestFixture.run_as_sync
    async def test_scanning(self):
        await self.system_clock_fast_forward(speed_multiplier=20)

        connection, error = await self.login('oreman', "127.0.0.1")
        self.assertIsNotNone(connection)
        commutator = connection.commutator
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        miner = modules.get_ship(commutator, "Miner", "miner-1")
        self.assertIsNotNone(miner)

        scanner = modules.get_celestial_scanner(miner, "scanner")
        self.assertIsNotNone(scanner)

        result = []
        errors = []

        def scanning_cb(objects, e):
            if objects:
                result.extend(objects)
            else:
                errors.append(e)

        self.assertIsNone(await scanner.scan(
            scanning_radius_km=20,
            minimal_radius_m=300,
            scanning_cb=scanning_cb))
        self.assertIsNone(error)
        self.assertEqual([], result)

        self.assertIsNone(await scanner.scan(
            scanning_radius_km=20,
            minimal_radius_m=200,
            scanning_cb=scanning_cb))
        self.assertIsNone(error)
        self.assertEqual(1, len(result))

    @BaseTestFixture.run_as_sync
    async def test_scanning_sync(self):
        await self.system_clock_fast_forward(speed_multiplier=20)

        connection, error = await self.login('oreman', "127.0.0.1")
        self.assertIsNotNone(connection)
        commutator = connection.commutator
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        miner = modules.get_ship(commutator, "Miner", "miner-1")
        self.assertIsNotNone(miner)

        scanner = modules.get_celestial_scanner(miner, "scanner")
        self.assertIsNotNone(scanner)

        result, error = await scanner.scan_sync(scanning_radius_km=20,
                                                minimal_radius_m=300)
        self.assertIsNone(error)
        self.assertEqual([], result)

        result, error = await scanner.scan_sync(scanning_radius_km=20,
                                                minimal_radius_m=200)
        self.assertIsNone(error)
        self.assertEqual(1, len(result))
