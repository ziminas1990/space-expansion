from base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world

from server.configurator.configuration import Configuration
from server.configurator.general import General, ApplicationMode

from expansion import modules
import asyncio


class TestGame(BaseTestFixture):

    def __init__(self, *args, **kwargs):
        super(TestGame, self).__init__(*args, **kwargs)

        self.configuration = Configuration(
            general=General(total_threads=1,
                            login_udp_port=7456,
                            initial_state=ApplicationMode.e_RUN,
                            ports_pool=(12000, 12100)),
            blueprints=blueprints.DefaultBlueprints(),
            world=world.World(),
            players={
                'player': world.Player(
                    login="player",
                    password="player",
                    ships=[]
                )
            }
        )

    def get_configuration(self) -> Configuration:
        return self.configuration

    @BaseTestFixture.run_as_sync
    async def test_game_is_a_root_module(self):
        # 1. player logins
        connection, error = await self.login('player', "127.0.0.1")
        self.assertIsNotNone(connection)
        self.assertIsNone(error)
        commutator = connection.commutator

        # 2. game is listed on the root commutator
        game = modules.get_game(commutator)
        self.assertIsNotNone(game)
        self.assertEqual("Game", game.name)

        # 3. start monitoring on a dedicated session
        updates = []

        async def collect():
            async for update in game.monitor():
                updates.append(update)

        task = asyncio.get_running_loop().create_task(collect())
        await asyncio.sleep(0.2)

        # 4. monitoring stays open until the session is closed
        self.assertFalse(task.done())
        self.assertEqual([], updates)

        # 5. closing the monitoring session stops updates
        task.cancel()
        try:
            await task
        except asyncio.CancelledError:
            pass
