import time
from typing import Optional, TYPE_CHECKING
import unittest
import asyncio
import abc
import datetime

from server.configurator.general import AdministratorCfg
from server.server import Server
from expansion.utils import set_asyncio_policy
import expansion.procedures as procedures
import expansion.modules as modules
import expansion.interfaces.privileged as privileged

if TYPE_CHECKING:
    from server.configurator.configuration import Configuration


class BaseTestFixture(unittest.TestCase):

    event_loop = None

    def __init__(self, *args, **kwargs):
        super(BaseTestFixture, self).__init__(*args, **kwargs)

        self.server: Server = Server()
        self.config: Optional["Configuration"] = None

        # Time management:
        self.time_wheel_task: Optional[asyncio.Task] = None
        self.time_manual_control_flag = False
        self.time_multiplier = 1
        self.time_granularity_us = 1000

    @abc.abstractmethod
    def get_configuration(self) -> "Configuration":
        pass

    async def async_setUp(self) -> None:
        self.administrator: privileged.Administrator = privileged.Administrator()

        self.config = self.get_configuration()
        if self.config.general.administrator_cfg is None:
            self.config.general.administrator_cfg = AdministratorCfg(
                udp_port=28000,
                login='administrator',
                password='iampower'
            )

        if self.config.general.global_grid is None:
            self.config.general.set_global_grid(
                grid_size=100, cell_width_km=1000
            )

        await self.server.run(self.config)
        await asyncio.sleep(0.5)

        admin_cfg = self.config.general.administrator_cfg
        status, error = await self.administrator.login(
            ip_address="127.0.0.1",
            port=admin_cfg.udp_port,
            login=admin_cfg.login,
            password=admin_cfg.password)

        if not status:
            # Since the administrator channel has not been opened,
            # there could be no other way to stop the server
            self.server.stop()
            assert status, error

    def setUp(self) -> None:
        set_asyncio_policy()
        BaseTestFixture.event_loop = asyncio.new_event_loop()
        asyncio.set_event_loop(BaseTestFixture.event_loop)

        BaseTestFixture.event_loop.run_until_complete(
            self.async_setUp()
        )

    def tearDown(self) -> None:
        async def stop_system_clock():
            """Stop the server's system clock (server will terminate)"""
            return await self.administrator.get_clock().terminate()
        BaseTestFixture.event_loop.run_until_complete(stop_system_clock())
        # At this point server should stop himself, but to be sure:
        self.server.stop()
        BaseTestFixture.event_loop = None

    async def login(self, player: str, server_ip: str, local_ip: str) \
            -> (Optional[modules.Commutator], str):
        general_cfg = self.config.general
        commutator = await procedures.login(
            server_ip=server_ip,
            login_port=general_cfg.login_udp_port,
            login=self.config.players[player].login,
            password=self.config.players[player].password,
            local_ip=local_ip)
        if not await commutator.init():
            # TODO: release commutators resources
            return None, "Failed to init root commutator"
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
    def run_as_sync(func):
        """This decorator turns coroutine in a regular method, that can be called
        without 'await' keyword."""
        def sync_runner(*args, **kwargs):
            return BaseTestFixture.event_loop.run_until_complete(func(*args, **kwargs))
        return sync_runner
