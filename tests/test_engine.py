import asyncio
from typing import List

from base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world
from server.configurator.modules import default_ships
from server.configurator.configuration import Configuration
from server.configurator.general import General, ApplicationMode

from expansion import modules
from expansion.types import Vector
import utils


class TestCase(BaseTestFixture):

    def __init__(self, *args, **kwargs):
        super(TestCase, self).__init__(*args, **kwargs)

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
                    ships=[
                        default_ships.make_probe(
                            name="scout-1",
                            position=world.Position(
                                x=0, y=0, velocity=world.Vector(0, 0))),
                    ]
                )
            },
        )

    def get_configuration(self) -> Configuration:
        return self.configuration

    async def _engine(self):
        connection, error = await self.login('player', "127.0.0.1")
        self.assertIsNotNone(connection)
        self.assertIsNone(error)
        ship = modules.get_ship(connection.commutator, "scout-1")
        self.assertIsNotNone(ship)
        engine = modules.get_engine(ship, "main_engine")
        self.assertIsNotNone(engine)
        return connection, engine

    async def _start_monitoring(self, engine):
        journal: List[Vector] = []
        started = False

        async def do_monitoring() -> None:
            nonlocal started
            async for thrust in engine.monitor():
                started = True
                if thrust is not None:
                    journal.append(thrust)

        task = asyncio.create_task(do_monitoring())
        await self._wait_journal(journal, 1)
        self.assertTrue(started)
        return journal, task

    async def _wait_journal(self, journal: List[Vector], count: int) -> None:
        deadline = asyncio.get_event_loop().time() + 2
        while len(journal) < count:
            self.assertLess(asyncio.get_event_loop().time(), deadline)
            proceeded, _ = await self.system_clock_proceed(50, timeout_s=2)
            self.assertTrue(proceeded)
            await asyncio.sleep(0.05)

    @BaseTestFixture.run_as_sync
    async def test_monitor_applied_thrust(self):
        # 1. player logins and gets the engine
        _, engine = await self._engine()
        success, _ = await self.system_clock_stop()
        self.assertTrue(success)

        # 2. start monitoring
        journal, task = await self._start_monitoring(engine)
        self.assertTrue(journal[0].almost_null())

        # 3. apply a new thrust vector
        expected = Vector(3, 4).set_length(100)
        self.assertTrue(await engine.set_thrust(expected, duration_ms=1_000_000))

        # 4. monitor receives the applied vector
        await self._wait_journal(journal, 2)
        self.assertTrue(Vector.almost_equal(journal[1], expected, delta=0.5))

        # 5. the same command produces another indication
        self.assertTrue(await engine.set_thrust(expected, duration_ms=1_000_000))
        await self._wait_journal(journal, 3)
        self.assertTrue(Vector.almost_equal(journal[2], expected, delta=0.5))

        task.cancel()
        await asyncio.wait([task])

    @BaseTestFixture.run_as_sync
    async def test_monitor_clamped_and_duration(self):
        # 1. player logins and gets the engine
        _, engine = await self._engine()
        spec = await engine.get_specification()
        self.assertIsNotNone(spec)
        success, _ = await self.system_clock_stop()
        self.assertTrue(success)

        # 2. start monitoring
        journal, task = await self._start_monitoring(engine)

        # 3. request a thrust above max_thrust
        direction = Vector(1, 0)
        self.assertTrue(await engine.set_thrust(
            direction.set_length(spec.max_thrust * 2, inplace=False),
            duration_ms=300))

        # 4. monitor receives the clamped vector
        await self._wait_journal(journal, 2)
        expected = direction.set_length(spec.max_thrust, inplace=False)
        self.assertTrue(Vector.almost_equal(journal[1], expected, delta=1.0))
        self.assertAlmostEqual(spec.max_thrust, journal[1].abs(), delta=1.0)

        # 5. when duration elapses, monitor receives zero thrust
        proceeded, _ = await self.system_clock_proceed(400, timeout_s=2)
        self.assertTrue(proceeded)
        self.assertTrue(await utils.wait_for(lambda: len(journal) >= 3, timeout=2))
        self.assertTrue(journal[2].almost_null())

        task.cancel()
        await asyncio.wait([task])

    @BaseTestFixture.run_as_sync
    async def test_monitor_delayed_thrust(self):
        # 1. player logins and gets the engine
        _, engine = await self._engine()
        success, now = await self.system_clock_stop()
        self.assertTrue(success)
        self.assertIsNotNone(now)

        # 2. start monitoring
        journal, task = await self._start_monitoring(engine)
        now = await self.system_clock_time()
        self.assertIsNotNone(now)

        # 3. schedule a delayed change_thrust
        expected = Vector(0, 1).set_length(80)
        self.assertTrue(await engine.set_thrust(
            expected, at=now + 200_000, duration_ms=1_000_000))

        # 4. no indication before the timestamp
        proceeded, _ = await self.system_clock_proceed(50, timeout_s=2)
        self.assertTrue(proceeded)
        await asyncio.sleep(0.2)
        self.assertEqual(1, len(journal))

        # 5. after the timestamp, monitor receives the applied vector
        proceeded, _ = await self.system_clock_proceed(300, timeout_s=2)
        self.assertTrue(proceeded)
        self.assertTrue(await utils.wait_for(lambda: len(journal) >= 2, timeout=2))
        self.assertTrue(Vector.almost_equal(journal[1], expected, delta=0.5))

        task.cancel()
        await asyncio.wait([task])
