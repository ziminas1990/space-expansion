import asyncio
import math
from typing import List
from dataclasses import dataclass

from base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world
from server.configurator.configuration import Configuration
from server.configurator.general import General, ApplicationMode

import expansion.modules as modules


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
        connection, error = await self.login('player', "127.0.0.1")
        self.assertIsNotNone(connection)
        commutator = connection.commutator
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
        self.assertEqual(time, current_time_point.usec())
        self.assertEqual(time, await system_clock.time(predict=False))

        # Checking wait_until feature
        wait_delta_ms = 10000
        task = asyncio.get_running_loop().create_task(
            system_clock.wait_until(
                time=current_time_point.usec() + wait_delta_ms * 1000,
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
        connection, error = await self.login('player', "127.0.0.1")
        self.assertIsNotNone(connection)
        commutator = connection.commutator
        self.assertIsNone(error)
        system_clock = modules.get_system_clock(commutator)
        self.assertIsNotNone(system_clock)

        success, time = await self.system_clock_stop()
        self.assertTrue(success)

        task_1 = asyncio.get_running_loop().create_task(
            system_clock.wait_for(period_us=2 * 10**6, timeout=1))
        task_2 = asyncio.get_running_loop().create_task(
            system_clock.wait_until(time=time + 10 * 10**6, timeout=1))
        task_3 = asyncio.get_running_loop().create_task(
            system_clock.wait_until(time=time + 5 * 10**6, timeout=1))
        task_4 = asyncio.get_running_loop().create_task(
            system_clock.wait_for(50 * 10**6, timeout=1))
        await asyncio.sleep(0.01)  # proceeding the loop to actually spawn a tasks

        # Proceed 2 seconds
        success, time = await self.system_clock_proceed(2*10**3, timeout_s=1)
        await asyncio.sleep(0.01)  # To exclude possible network latency influence
        self.assertTrue(success)
        self.assertTrue(task_1.done())

        task_1 = asyncio.get_running_loop().create_task(
            system_clock.wait_for(period_us=3 * 10**6, timeout=1))

        # Proceed 3 more seconds
        success, time = await self.system_clock_proceed(3 * 10**3, timeout_s=1)
        await asyncio.sleep(0.1)  # To exclude possible network latency influence
        self.assertTrue(success)
        self.assertTrue(task_3.done())
        self.assertTrue(task_1.done())

        # Proceed 5 more seconds
        success, time = await self.system_clock_proceed(5 * 10**3, timeout_s=1)
        await asyncio.sleep(0.01)  # To exclude possible network latency influence
        self.assertTrue(success)
        self.assertTrue(task_2.done())

        # Finally, proceed 46 seconds
        success, time = await self.system_clock_proceed(46 * 10**3, timeout_s=5)
        await asyncio.sleep(0.01)  # To exclude possible network latency influence
        self.assertTrue(success)
        self.assertTrue(task_4.done())

    @BaseTestFixture.run_as_sync
    async def test_monitoring(self):
        await self.system_clock_fast_forward(10)

        connection, error = await self.login('player', "127.0.0.1")
        self.assertIsNotNone(connection)
        commutator = connection.commutator
        self.assertIsNone(error)
        system_clock = modules.get_system_clock(commutator)
        self.assertIsNotNone(system_clock)

        @dataclass
        class Session:
            interval: int
            timestamps: List[int]
            task = None

        async def start_monitoring(session: Session):
            # Mutex is used to prevent creating of a number UDP connections
            async for timestamps in system_clock.monitor(session.interval):
                session.timestamps.append(timestamps)

        sessions = [Session(110, []),
                    Session(75, []),
                    Session(55, []),
                    Session(20, [])]

        # Run all monitoring sessions
        _, started_at = await self.system_clock_stop()
        for session in sessions:
            session.task = asyncio.create_task(start_monitoring(session))

        # Wait 10 seconds of ingame time
        await self.system_clock_fast_forward(speed_multiplier=5)
        await system_clock.wait_for(10 * 10**6, timeout=5)
        _, end_at = await self.system_clock_stop()

        for session in sessions:
            session.task.cancel()

        for session in sessions:
            await session.task
            total_timestamps = len(session.timestamps)
            self.assertGreater(total_timestamps, 0)
            session_duration = (end_at - session.timestamps[0]) / 1000
            total_expected = 1 + math.floor(session_duration / session.interval)
            self.assertEqual(total_expected, total_timestamps)
            for i in range(1, total_timestamps):
                self.assertEqual(
                    session.interval * 1000,
                    session.timestamps[i] - session.timestamps[i-1])
