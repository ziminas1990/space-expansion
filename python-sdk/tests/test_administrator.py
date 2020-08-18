import time

from .base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world
from server.configurator import (Configuration, General, ApplicationMode)


class TestSystemClock(BaseTestFixture):

    def __init__(self, *args, **kwargs):
        super(TestSystemClock, self).__init__(*args, **kwargs)

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
            assert ingame_time is not None
            self.assertEqual(0, ingame_time)
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
        time.sleep(0.1)
        status, ingame_time = await self.system_clock_stop()
        self.assertTrue(status)
        # Rude check with 10% accuracy:
        self.assertAlmostEqual(100000, ingame_time, delta=10000)
