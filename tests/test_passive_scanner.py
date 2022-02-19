from base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world

from server.configurator.modules import default_ships, ShipType
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
            world=world.World(),
        )

    def get_configuration(self) -> Configuration:
        return self.configuration

    @BaseTestFixture.run_as_sync
    async def test_get_specification(self):
        await self.system_clock_fast_forward(speed_multiplier=20)

        commutator, error = await self.login('oreman', "127.0.0.1", "127.0.0.1")
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        miner_1 = modules.get_ship(commutator, ShipType.MINER.value, "miner-1")
        self.assertIsNotNone(miner_1)

        scanner = modules.PassiveScanner.get_by_name(miner_1, "perceiver")
        self.assertIsNotNone(scanner)

        spec = await scanner.get_specification()
        self.assertIsNotNone(spec)
