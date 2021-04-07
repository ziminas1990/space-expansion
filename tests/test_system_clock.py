import asyncio
import logging

from base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world
from server.configurator import Configuration, General, ApplicationMode

import expansion.modules as modules
import expansion.types as types


class TestSystemClock(BaseTestFixture):

    def __init__(self, *args, **kwargs):
        super(TestSystemClock, self).__init__(*args, **kwargs)

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
    async def test_breath(self):
        commutator, error = await self.login(player='player',
                                             server_ip="127.0.0.1",
                                             local_ip="127.0.0.1")
        self.assertIsNone(error)
        system_clock = modules.get_system_clock(commutator)
        self.assertIsNotNone(system_clock)

        # Checking time_req function
        success, time = await self.system_clock_stop()
        self.assertTrue(success)
        current_time = await system_clock.time(predict=False)
        self.assertEqual(time, current_time)

        current_time_point = system_clock.time_point()
        success, time = await self.system_clock_proceed(1000, timeout_s=0.2)
        self.assertTrue(success)
        await system_clock.sync()
        # sync() should update the current_time_point object
        self.assertEqual(time, current_time_point.now(predict=False))
        self.assertEqual(time, await system_clock.time(predict=False))

        # Checking wait_until feature
        wait_delta_ms = 10000
        task = asyncio.get_running_loop().create_task(
            system_clock.wait_until(
                time=current_time_point.now(predict=False) + wait_delta_ms * 1000,
                timeout=1))

        success, time = await self.system_clock_proceed(wait_delta_ms - 1, timeout_s=1)
        await asyncio.sleep(0.01)  # To exclude possible network latency influence
        self.assertTrue(success)
        self.assertFalse(task.done())

        success, time = await self.system_clock_proceed(1, timeout_s=1)
        await asyncio.sleep(0.01)  # To exclude possible network latency influence
        self.assertTrue(success)
        self.assertTrue(task.done())

        # Checking wait_for feature
        task = asyncio.get_running_loop().create_task(
            system_clock.wait_for(period_us=wait_delta_ms * 1000, timeout=1)
        )

        success, time = await self.system_clock_proceed(wait_delta_ms - 1, timeout_s=1)
        await asyncio.sleep(0.01)  # To exclude possible network latency influence
        self.assertTrue(success)
        self.assertFalse(task.done())

        success, time = await self.system_clock_proceed(1, timeout_s=1)
        await asyncio.sleep(0.01)  # To exclude possible network latency influence
        self.assertTrue(success)
        self.assertTrue(task.done())

    @BaseTestFixture.run_as_sync
    async def test_multiple_sessions(self):
        commutator, error = await self.login(player='player',
                                             server_ip="127.0.0.1",
                                             local_ip="127.0.0.1")
        self.assertIsNone(error)
        system_clock = modules.get_system_clock(commutator)
        self.assertIsNotNone(system_clock)

        success, time = await self.system_clock_stop()
        self.assertTrue(success)

        task_1 = asyncio.get_running_loop().create_task(
            system_clock.wait_for(period_us=20000, timeout=1))
        task_2 = asyncio.get_running_loop().create_task(
            system_clock.wait_until(time=time + 100000, timeout=1))
        task_3 = asyncio.get_running_loop().create_task(
            system_clock.wait_until(time=time + 50000, timeout=1))
        task_4 = asyncio.get_running_loop().create_task(
            system_clock.wait_for(500000, timeout=1))
        await asyncio.sleep(0.01)  # proceeding the loop to actually spawn a tasks

        success, time = await self.system_clock_proceed(20, timeout_s=1)
        await asyncio.sleep(0.01)  # To exclude possible network latency influence
        self.assertTrue(success)
        self.assertTrue(task_1.done())

        task_1 = asyncio.get_running_loop().create_task(
            system_clock.wait_for(period_us=30000, timeout=1))

        success, time = await self.system_clock_proceed(30, timeout_s=1)
        await asyncio.sleep(0.01)  # To exclude possible network latency influence
        self.assertTrue(success)
        self.assertTrue(task_3.done())
        self.assertTrue(task_1.done())

        success, time = await self.system_clock_proceed(50, timeout_s=1)
        await asyncio.sleep(0.01)  # To exclude possible network latency influence
        self.assertTrue(success)
        self.assertTrue(task_2.done())

        success, time = await self.system_clock_proceed(450, timeout_s=1)
        await asyncio.sleep(0.01)  # To exclude possible network latency influence
        self.assertTrue(success)
        self.assertTrue(task_4.done())

    @BaseTestFixture.run_as_sync
    async def test_generator(self):
        await self.system_clock_fast_forward(10)

        commutator, error = await self.login(player='player',
                                             server_ip="127.0.0.1",
                                             local_ip="127.0.0.1")
        self.assertIsNone(error)
        system_clock = modules.get_system_clock(commutator)
        self.assertIsNotNone(system_clock)

        generator_tick = await system_clock.get_generator_tick_us()
        self.assertIsNotNone(generator_tick)
        current_time = types.TimePoint(0)

        def time_cb(time: types.TimePoint):
            nonlocal current_time
            current_time = time
            logging.debug(f"on time_cb() with time={current_time}")

        system_clock.subscribe(time_cb)

        for i in range(10):
            success, time = await self.system_clock_proceed(500, timeout_s=1)
            await asyncio.sleep(0.02)  # waiting for all events to be handled
            delta = time - current_time.now(predict=False)
            self.assertLessEqual(delta, generator_tick)
