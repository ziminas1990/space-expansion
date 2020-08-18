from tests.base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world
import server.configurator.modules.default_ships as default_ships
from server.configurator import Configuration, General, ApplicationMode

import expansion.procedures as procedures


class TestNavigation(BaseTestFixture):

    def __init__(self, *args, **kwargs):
        super(TestNavigation, self).__init__(*args, **kwargs)

        self.configuration = Configuration(
            general=General(total_threads=1,
                            login_udp_port=7456,
                            initial_state=ApplicationMode.e_RUN,
                            ports_pool=(12000, 12100)),
            blueprints=blueprints.DefaultBlueprints(),
            world=world.World(),
            players={
                'spy007': world.Player(
                    login="spy007",
                    password="iamspy",
                    ships=[
                        default_ships.make_probe(
                            name="scout-1",
                            position=world.Position(
                                x=100, y=200, velocity=world.Vector(100, -100))),
                        default_ships.make_probe(
                            name="scout-2",
                            position=world.Position(
                                x=-100, y=-200, velocity=world.Vector(-10, 20)))
                    ]
                )
            }
        )

    def get_configuration(self) -> Configuration:
        return self.configuration

    @BaseTestFixture.run_as_sync
    async def test_move_to(self):
        commutator, error = await self.login('spy007')
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        scout_1 = await procedures.connect_to_ship("Probe", "scout-1", commutator)
        self.assertIsNotNone(scout_1)

        scout_2 = await procedures.connect_to_ship("Probe", "scout-2", commutator)
        self.assertIsNotNone(scout_2)

        engine = await procedures.connect_to_engine(name='main_engine', ship=scout_1)
        self.assertIsNotNone(engine)

        async def target() -> world.Position:
            return await scout_2.get_navigation().get_position()

        await procedures.move_to(ship=scout_1,
                                 engine=engine,
                                 position=target,
                                 max_distance_error=5,
                                 max_velocity_error=1,
                                 cb_sleep=self.ingame_sleep)

        scout_1_position = await scout_1.get_navigation().get_position()
        scout_2_position = await scout_2.get_navigation().get_position()
        delta = scout_1_position.distance_to(scout_2_position)
        self.assertTrue(delta < 10)
