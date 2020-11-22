from typing import Optional, Callable, Awaitable

from base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world
import server.configurator.modules.default_ships as default_ships
from server.configurator import Configuration, General, ApplicationMode

import expansion.interfaces.public as rpc
import expansion.modules as modules
import expansion.procedures as procedures


class FastForwardSystemClock(rpc.SystemClockI):
    def __init__(self, system_clock: modules.SystemClock,
                 switch_to_real_time: Callable[[], Awaitable[bool]],
                 fast_forward: Callable[[int, int], Awaitable[None]],
                 fast_forward_multiplier: int):
        super().__init__(name="FastForwardSystemClock")
        self.system_clock = system_clock
        self.switch_to_real_time = switch_to_real_time
        self.fast_forward = fast_forward
        self.fast_forward_multiplier = fast_forward_multiplier

    async def wait_until(self, time: int, timeout: float) -> Optional[int]:
        """Wait until server time reaches the specified 'time'"""
        await self.fast_forward(self.fast_forward_multiplier)
        result = await self.system_clock.wait_until(time, timeout)
        await self.switch_to_real_time()
        return result

    async def wait_for(self, period_us: int, timeout: float) -> Optional[int]:
        """Wait for the specified 'period' microseconds"""
        await self.fast_forward(self.fast_forward_multiplier)
        result = await self.system_clock.wait_for(period_us, timeout)
        await self.switch_to_real_time()
        return result


class TestNavigation(BaseTestFixture):

    def __init__(self, *args, **kwargs):
        super(TestNavigation, self).__init__(*args, **kwargs)

        self.configuration = Configuration(
            general=General(total_threads=1,
                            login_udp_port=7456,
                            initial_state=ApplicationMode.e_RUN,
                            ports_pool=(12000, 12100)),
            blueprints=blueprints.DefaultBlueprints(),
            world=world.World(),
            players={
                'spy007': world.Player(
                    login="spy007",
                    password="iamspy",
                    ships=[
                        default_ships.make_probe(
                            name="scout-1",
                            position=world.Position(
                                x=100, y=200, velocity=world.Vector(100, -100))),
                        default_ships.make_probe(
                            name="scout-2",
                            position=world.Position(
                                x=-100, y=-200, velocity=world.Vector(-10, 20)))
                    ]
                )
            }
        )

    def get_configuration(self) -> Configuration:
        return self.configuration

    @BaseTestFixture.run_as_sync
    async def test_move_to(self):
        commutator, error = await self.login('spy007',
                                             server_ip="127.0.0.1",
                                             local_ip="127.0.0.1")
        self.assertIsNone(error)

        system_clock = modules.get_system_clock(commutator)
        self.assertIsNotNone(system_clock)
        fast_forward_clock = FastForwardSystemClock(
            system_clock=system_clock,
            switch_to_real_time=self.system_clock_play,
            fast_forward=self.system_clock_fast_forward,
            fast_forward_multiplier=50)

        scout_1: modules.Ship = modules.get_ship(commutator, "Probe", "scout-1")
        self.assertIsNotNone(scout_1)
        scout_2: modules.Ship = modules.get_ship(commutator, "Ship/Probe", "scout-2")
        self.assertIsNotNone(scout_1)

        engine = modules.get_engine(scout_1, "main_engine")
        self.assertIsNotNone(engine)

        success = await procedures.move_to(
            ship=scout_1,
            engine=engine,
            position=scout_2.get_position,
            system_clock=fast_forward_clock,
            max_distance_error=5,
            max_velocity_error=1)

        self.assertTrue(success)
        scout_1_position = await scout_1.get_position()
        scout_2_position = await scout_2.get_position()
        delta = scout_1_position.distance_to(scout_2_position)
        self.assertTrue(delta < 5)
