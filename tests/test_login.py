import asyncio
import logging

from base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world
import server.configurator.modules.default_ships as default_ships
from server.configurator.general import General, ApplicationMode
from server.configurator.resources import ResourceType
from server.configurator.configuration import Configuration


class TestLogin(BaseTestFixture):

    def __init__(self, *args, **kwargs):
        super(TestLogin, self).__init__(*args, **kwargs)

        self.configuration = Configuration(
            general=General(total_threads=1,
                            login_udp_port=7456,
                            initial_state=ApplicationMode.e_RUN,
                            ports_pool=(12000, 12100)),
            blueprints=blueprints.DefaultBlueprints(),
            world=world.World(
                asteroids=world.Asteroids(asteroids=[
                    world.Asteroid(position=world.Position(x=10000, y=10000),
                                   radius=200,
                                   composition={ResourceType.e_ICE: 20,
                                                ResourceType.e_SILICATES: 20,
                                                ResourceType.e_METALS: 20})
                ])),
            players={
                'spy007': world.Player(
                    login="spy007",
                    password="iamspy",
                    ships=[
                        default_ships.make_probe(name="scout-1", position=world.Position(100, 200))
                    ]
                )
            }
        )

    def get_configuration(self) -> Configuration:
        return self.configuration

    @BaseTestFixture.run_as_sync
    async def test_login(self):
        commutator, error = \
            await self.login("spy007", "127.0.0.1")
        self.assertIsNone(error)
        self.assertIsNotNone(commutator)

    @BaseTestFixture.run_as_sync
    async def test_logins_in_turns(self):
        for i in range(8):  # 8 is a hardcoded server's limit
            logging.info(f"Iteration {i}")
            commutator, error = \
                await self.login("spy007", "127.0.0.1")
            self.assertIsNotNone(commutator, f"{error} at iteration {i}")
