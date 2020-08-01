from typing import Optional
import unittest
import asyncio
import abc

from server.configurator import Configuration, AdministratorCfg
from server.server import Server

import expansion.procedures as procedures
import expansion.interfaces.public as modules
import expansion.interfaces.privileged as privileged


class BaseTestFixture(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super(BaseTestFixture, self).__init__(*args, **kwargs)
        self.server: Server = Server()
        self.login_ports = list(range(5000, 5100))
        self.administrator: privileged.Administrator = privileged.Administrator()
        self.config: Optional[Configuration] = None

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
        assert status, error

    def tearDown(self) -> None:
        @BaseTestFixture.run_as_sync
        async def stop_system_clock():
            """Stop the server's system clock (server will terminate)"""
            return await self.administrator.get_clock().terminate()
        stop_system_clock()
        # At this point server should stop himself, but to be sure:
        self.server.stop()

    async def login(self, player: str) -> (Optional[modules.Commutator], Optional[str]):
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

    async def ingame_time(self) -> (bool, Optional[int]):
        """Return current in-game time"""
        return True, await self.administrator.get_clock().get_time()

    async def freeze(self) -> (bool, Optional[int]):
        """
        Switches system clock to debug mode and return current time.
        Return True and current in-game time on success, and False on error
        """
        if not await self.administrator.get_clock().switch_to_debug_mode():
            return False, None
        return True, await self.administrator.get_clock().get_time()

    async def continue_in_real_time(self) -> (bool, Optional[int]):
        """
        Switches system clock to real time mode and return current time.
        Return True and current in-game time on success, and False on error
        """
        if not await self.administrator.get_clock().switch_to_real_time():
            return False, None
        return True, await self.administrator.get_clock().get_time()

    async def proceed_time(self,
                           proceed_milliseconds: int,
                           timeout_sec: float,
                           tick_duration_usec: int = 1000) -> (bool, int):
        """
        Proceed the specified amount of 'proceed_milliseconds' milliseconds. Each tick
        will be a 'tick_duration_usec' microseconds long.
        On success return True and current in-game time (after it has been proceeded).
        Otherwise return False and 0
        """
        proceed_usec = proceed_milliseconds * 1000
        if not await self.administrator.get_clock().set_tick_duration(tick_duration_usec):
            return False, 0
        ticks: int = int(proceed_usec / tick_duration_usec)
        if proceed_usec % tick_duration_usec > 0:
            ticks += 1  # excessively is better than insufficiently
        return await self.administrator.get_clock().proceed_ticks(ticks, timeout_sec)

    async def fast_forward(self,
                           proceed_milliseconds: int,
                           timeout_sec: float,
                           tick_duration_usec: int = 1000):
        """Same as 'proceed_time', but assumes that system clock is in
        REAL_TIME mode"""
        # Switching system clock to the debug mode, proceeding time and
        # switching clock back to the real-time mode
        success, time = await self.freeze()
        self.assertTrue(success)

        success, new_time = await self.proceed_time(proceed_milliseconds,
                                                    timeout_sec,
                                                    tick_duration_usec)
        self.assertTrue(success)
        # +- tick error is ok
        self.assertAlmostEqual(time + proceed_milliseconds * 1000, new_time,
                               delta=tick_duration_usec)

        success, _ = await self.continue_in_real_time()
        self.assertTrue(success)

    async def sleep_in_game(self, time_sec: float):
        """Same as 'fast_forward', but requires only a single parameter.
        May be passed as a 'sleep' callback"""
        time_milliseconds = int(time_sec * 1000)
        # Assume that fast-forward should be at least 25 times faster
        timeout_sec: float = time_sec / 25
        if timeout_sec < 1:
            timeout_sec = 1
        await self.fast_forward(proceed_milliseconds=time_milliseconds,
                                timeout_sec=timeout_sec)

    @staticmethod
    def run_as_sync(test_func):
        """This decorator turns coroutine in a regular method, that can be called
        without 'await' keyword."""
        def sync_runner(*args, **kwargs):
            return asyncio.get_event_loop().run_until_complete(test_func(*args, **kwargs))
        return sync_runner
