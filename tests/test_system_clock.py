import asyncio
import logging

from base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world
from server.configurator import Configuration, General, ApplicationMode

import expansion.procedures as procedures
from expansion.interfaces.public import SystemClock


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
        commutator, error = await self.login('player')
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        system_clock = await procedures.connect_to_system_clock(commutator)
        self.assertIsNotNone(system_clock)

        # Checking time_req function
        success, time = await self.system_clock_stop()
        self.assertTrue(success)
        current_time = await system_clock.time()
        self.assertEqual(time, current_time)

        success, time = await self.system_clock_proceed(1000, timeout_s=0.2)
        self.assertTrue(success)
        current_time = await system_clock.time()
        self.assertEqual(time, current_time)

        wait_delta_ms = 10000

        # Checking wait_until feature
        task = asyncio.get_running_loop().create_task(
            system_clock.wait_until(time=current_time + wait_delta_ms * 1000, timeout=1))

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
        commutator, error = await self.login('player')
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        system_clock_1 = await procedures.connect_to_system_clock(commutator)
        self.assertIsNotNone(system_clock_1)
        system_clock_2 = await procedures.connect_to_system_clock(commutator)
        self.assertIsNotNone(system_clock_2)
        system_clock_3 = await procedures.connect_to_system_clock(commutator)
        self.assertIsNotNone(system_clock_3)
        system_clock_4 = await procedures.connect_to_system_clock(commutator)
        self.assertIsNotNone(system_clock_4)

        success, time = await self.system_clock_stop()
        self.assertTrue(success)

        task_1 = asyncio.get_running_loop().create_task(
            system_clock_2.wait_for(period_us=20000, timeout=1))
        task_2 = asyncio.get_running_loop().create_task(
            system_clock_3.wait_until(time=time + 100000, timeout=1))
        task_3 = asyncio.get_running_loop().create_task(
            system_clock_1.wait_until(time=time + 50000, timeout=1))
        task_4 = asyncio.get_running_loop().create_task(
            system_clock_4.wait_for(500000, timeout=1))

        success, time = await self.system_clock_proceed(20, timeout_s=1)
        await asyncio.sleep(0.01)  # To exclude possible network latency influence
        self.assertTrue(success)
        self.assertTrue(task_1.done())

        task_1 = asyncio.get_running_loop().create_task(
            system_clock_2.wait_for(period_us=30000, timeout=1))

        success, time = await self.system_clock_proceed(30, timeout_s=1)
        await asyncio.sleep(0.01)  # To exclude possible network latency influence
        self.assertTrue(success)
        self.assertTrue(task_3.done())
        self.assertTrue(task_1.done())

        success, time = await self.system_clock_proceed(50, timeout_s=1)
        await asyncio.sleep(0.01)  # To exclude possible network latency influence
        self.assertTrue(success)
        self.assertTrue(task_2.done())

        success, time = await self.system_clock_proceed(400, timeout_s=1)
        await asyncio.sleep(0.01)  # To exclude possible network latency influence
        self.assertTrue(success)
        self.assertTrue(task_4.done())

    @BaseTestFixture.run_as_sync
    async def test_generator(self):
        await self.system_clock_fast_forward(10)

        commutator, error = await self.login('player')
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        system_clock = await procedures.connect_to_system_clock(commutator)
        self.assertIsNotNone(system_clock)

        generator_tick = await system_clock.get_generator_tick_us()
        self.assertIsNotNone(generator_tick)
        current_time = 0

        def time_cb(time: int):
            nonlocal current_time
            current_time = time
            logging.debug(f"on time_cb() with time={current_time}")

        status = await system_clock.attach_to_generator(time_cb)
        self.assertEqual(status, SystemClock.Status.SUCCESS)

        for i in range(10):
            success, time = await self.system_clock_proceed(500, timeout_s=1)
            await asyncio.sleep(0.02)  # waiting for all events to be handled
            delta = time - current_time
            self.assertLessEqual(delta, generator_tick)