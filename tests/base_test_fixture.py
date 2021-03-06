from typing import Optional
import unittest
import asyncio
import abc
import datetime

from server.configurator import Configuration, AdministratorCfg
from server.server import Server

import expansion.procedures as procedures
import expansion.interfaces.public as rpc
import expansion.modules as modules
import expansion.interfaces.privileged as privileged


class BaseTestFixture(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super(BaseTestFixture, self).__init__(*args, **kwargs)
        self.server: Server = Server()
        self.login_ports = list(range(5000, 5100))
        self.administrator: privileged.Administrator = privileged.Administrator()
        self.config: Optional[Configuration] = None

        # Time management:
        self.time_wheel_task: Optional[asyncio.Task] = None
        self.time_manual_control_flag = False
        self.time_multiplier = 1
        self.time_granularity_us = 1000

    @abc.abstractmethod
    def get_configuration(self) -> Configuration:
        pass

    def setUp(self) -> None:
        self.config = self.get_configuration()
        if self.config.general.administrator_cfg is None:
            self.config.general.administrator_cfg = AdministratorCfg(
                udp_port=28000,
                login='administrator',
                password='iampower'
            )

        self.server.run(self.config)

        @BaseTestFixture.run_as_sync
        async def login_as_administrator() -> (bool, Optional[str]):
            """Login as administrator"""
            admin_cfg = self.config.general.administrator_cfg
            return await self.administrator.login(
                ip_address="127.0.0.1",
                port=admin_cfg.udp_port,
                login=admin_cfg.login,
                password=admin_cfg.password)

        status, error = login_as_administrator()
        if not status:
            # Since the administrator channel has not been opened,
            # there could be no other way to stop the server
            self.server.stop()
            assert status, error

    def tearDown(self) -> None:
        @BaseTestFixture.run_as_sync
        async def stop_system_clock():
            """Stop the server's system clock (server will terminate)"""
            return await self.administrator.get_clock().terminate()
        stop_system_clock()
        # At this point server should stop himself, but to be sure:
        self.server.stop()

    async def login_old(self, player: str) -> (Optional[rpc.CommutatorI], Optional[str]):
        if player not in self.config.players:
            return None, f"Player {player} doesn't exist!"
        general_cfg = self.config.general
        port = self.login_ports.pop()
        return await procedures.login(
            server_ip="127.0.0.1",
            login_port=general_cfg.login_udp_port,
            login=self.config.players[player].login,
            password=self.config.players[player].password,
            local_ip="127.0.0.1",
            local_port=port)

    async def login(self, player: str, server_ip: str, local_ip: str) \
            -> (Optional[modules.Commutator], Optional[str]):
        general_cfg = self.config.general
        port = self.login_ports.pop()
        commutator = modules.RootCommutator(name="Root")
        error = await commutator.login(
            server_ip=server_ip,
            login_port=general_cfg.login_udp_port,
            login=self.config.players[player].login,
            password=self.config.players[player].password,
            local_ip=local_ip,
            local_port=port)
        if error is not None or not await commutator.init():
            return None, error
        return commutator, None

    async def _proceed_time(self, proceed_ms: int, timeout_s: float) -> (bool, Optional[int]):
        """
        Proceed the specified amount of 'proceed_ms' milliseconds. Each tick
        will be a 'granularity_us' microseconds long.
        On success return True and current in-game time (after it has been proceeded).
        Otherwise return False and 0
        """
        proceed_us = proceed_ms * 1000
        ticks: int = int(proceed_us / self.time_granularity_us)
        if proceed_us % self.time_granularity_us > 0:
            ticks += 1  # excessively is better than insufficiently
        return await self.administrator.get_clock().proceed_ticks(ticks, timeout_s)

    async def _wheel_of_time(self) -> bool:
        """Control time flow on the server"""
        # Time can be managed only in the FREEZE mode:
        ingame_begin_us = await self.administrator.get_clock().get_time()
        ingame_now_us = ingame_begin_us
        assert ingame_begin_us is not None, "Failed to get current ingame time"

        begin = datetime.datetime.now()

        def time_passed_ms():
            return int((datetime.datetime.now() - begin).total_seconds() * 1000)

        if not await self.administrator.get_clock().set_tick_duration(self.time_granularity_us):
            return False

        while self.time_manual_control_flag:
            delta_ms = time_passed_ms()
            ingame_delta_ms = int((ingame_now_us - ingame_begin_us) / 1000)
            proceed_interval_ms = self.time_multiplier * delta_ms - ingame_delta_ms
            if proceed_interval_ms < 1:
                await asyncio.sleep(0.01)
                continue

            success, ingame_now_us = await self._proceed_time(
                proceed_ms=proceed_interval_ms,
                timeout_s=2 * proceed_interval_ms)
            assert success
            if not success:
                return False
        return True

    async def system_clock_stop(self) -> (bool, Optional[int]):
        """Switch system clock to the FREEZE state.
        Return True and current in-game time on success, and False on error"""
        if self.time_manual_control_flag is True:
            assert self.time_wheel_task
            self.time_manual_control_flag = False
            assert await asyncio.wait([self.time_wheel_task])
            self.time_wheel_task = None
        if not await self.administrator.get_clock().switch_to_debug_mode():
            return False, None
        return True, await self.administrator.get_clock().get_time()

    async def system_clock_play(self) -> bool:
        """Switch server to the real-time mode"""
        return await self.system_clock_stop() and \
            await self.administrator.get_clock().switch_to_real_time()

    async def system_clock_fast_forward(self,
                                        speed_multiplier: int = 1,
                                        granularity_us: int = 1000):
        """Switch system clock to freeze state and proceed it manually with the
        specified 'speed_multiplier' and 'granularity_us'"""
        success, _ = await self.system_clock_stop()
        assert success
        self.time_multiplier = speed_multiplier
        self.time_granularity_us = granularity_us
        self.time_manual_control_flag = True
        self.time_wheel_task = asyncio.get_event_loop().create_task(self._wheel_of_time())

    async def system_clock_proceed(self,
                                   proceed_ms: int,
                                   timeout_s: float,
                                   granularity_us: int = 1000) -> (bool, int):
        """
        Proceed the specified amount of 'proceed_ms' milliseconds. Each tick
        will be a 'granularity_us' microseconds long.
        On success return True and current in-game time (after it has been proceeded).
        Otherwise return False and 0
        """
        success, _ = await self.system_clock_stop()
        if not success:
            return False, 0
        self.time_granularity_us = granularity_us
        if not await self.administrator.get_clock().set_tick_duration(self.time_granularity_us):
            return False, 0
        return await self._proceed_time(proceed_ms, timeout_s)

    async def system_clock_time(self) -> Optional[int]:
        """Return current ingame time"""
        return await self.administrator.system_clock.get_time()

    async def ingame_sleep(self, time_s: float):
        """Same as 'system_clock_proceed' but requires only a single parameter,
        so it may be used instead of asyncio.sleep()"""
        success, time = await self.system_clock_proceed(
            proceed_ms=int(time_s * 1000),
            timeout_s=time_s,
            granularity_us=self.time_granularity_us)

    @staticmethod
    def run_as_sync(test_func):
        """This decorator turns coroutine in a regular method, that can be called
        without 'await' keyword."""
        def sync_runner(*args, **kwargs):
            return asyncio.get_event_loop().run_until_complete(test_func(*args, **kwargs))
        return sync_runner
