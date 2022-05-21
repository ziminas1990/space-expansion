import time

from base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world
from server.configurator.configuration import Configuration
from server.configurator.general import General, ApplicationMode
from randomizer import Randomizer
import expansion.types as types


class TestCase(BaseTestFixture):

    def __init__(self, *args, **kwargs):
        super(TestCase, self).__init__(*args, **kwargs)

        self.configuration = Configuration(
            general=General(total_threads=1,
                            login_udp_port=7456,
                            initial_state=ApplicationMode.e_FREEZE,
                            ports_pool=(12000, 12100)),
            blueprints=blueprints.DefaultBlueprints(),
            world=world.World(),
            players={
                'spy007': world.Player(
                    login="spy007",
                    password="iamspy",
                    ships=[]
                )
            }
        )

    def get_configuration(self) -> Configuration:
        return self.configuration

    @BaseTestFixture.run_as_sync
    async def test_time_freezed(self):
        for i in range(3):
            ingame_time = await self.system_clock_time()
            self.assertIsNotNone(ingame_time, f"Iteration {i}")
            self.assertEqual(0, ingame_time, f"Iteration {i}")
            time.sleep(0.1)

    @BaseTestFixture.run_as_sync
    async def test_time_proceed(self):
        ingame_time = await self.system_clock_time()
        assert ingame_time is not None
        self.assertEqual(0, ingame_time)

        status, new_ingame_time = await self.system_clock_proceed(proceed_ms=2000, timeout_s=1)
        self.assertTrue(status)
        time_delta = new_ingame_time - ingame_time
        self.assertAlmostEqual(2000000, time_delta, delta=1000)

    @BaseTestFixture.run_as_sync
    async def test_switch_mode(self):
        ingame_time = await self.system_clock_time()
        assert ingame_time is not None
        self.assertEqual(0, ingame_time)

        self.assertTrue(await self.system_clock_play())
        time.sleep(1)
        status, ingame_time = await self.system_clock_stop()
        self.assertTrue(status)
        # Rude check with 5% accuracy:
        self.assertAlmostEqual(1000000, ingame_time, delta=50000)

    @BaseTestFixture.run_as_sync
    async def test_spawn_asteroids(self):
        """Check that spawn.asteroid request returns asteroid id"""
        self.assertTrue(await self.system_clock_play())
        randomizer = Randomizer(2132)
        spawner = self.administrator.spawner
        for i in range(100):
            now = await self.system_clock_time()
            asteroid = await spawner.spawn_asteroid(
                position=randomizer.random_position(
                    rect=types.Rect(-1000, 1000, -1000, 1000),
                    min_speed=0,
                    max_speed=1000
                ),
                composition=types.make_resources(ice=100, metals=32),
                radius=10
            )
            assert asteroid is not None
            assert asteroid.position.timestamp.usec() >= now
