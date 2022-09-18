import asyncio
from typing import List, Dict, Optional
from base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world

from server.configurator.configuration import Configuration
from server.configurator.general import General, ApplicationMode

from expansion import modules
from expansion import types
from expansion.interfaces import rpc

from randomizer import Randomizer
import utils as utils


class TestCase(BaseTestFixture):

    def __init__(self, *args, **kwargs):
        super(TestCase, self).__init__(*args, **kwargs)

        self.configuration = Configuration(
            general=General(total_threads=1,
                            login_udp_port=7456,
                            initial_state=ApplicationMode.e_RUN,
                            ports_pool=(12000, 12100)),
            blueprints=blueprints.DefaultBlueprints(),
            players={
                'player': world.Player(
                    login="player",
                    password="expansion",
                    ships=[]
                )
            },
            world=world.World(),
        )

    def get_configuration(self) -> Configuration:
        return self.configuration

    @BaseTestFixture.run_as_sync
    async def test_monitoring(self):
        randomizer = Randomizer(4934)
        connection, error = await self.login('player', "127.0.0.1")
        self.assertIsNotNone(connection)
        commutator = connection.commutator
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        # Commutator has no ships attached
        ships = modules.Ship.get_all_ships(commutator)
        self.assertEqual(ships, [])

        monitoring_started: bool = False
        updates: List[rpc.CommutatorUpdate] = []

        async def wait_for_update(timeout: float = 1) \
                -> Optional[rpc.CommutatorUpdate]:
            if await utils.wait_for(lambda: len(updates) > 0, timeout):
                return updates.pop(0)
            return None

        async def monitoring() -> bool:
            nonlocal monitoring_started
            async for update in commutator.monitoring():
                monitoring_started = True
                if update is not None:
                    assert isinstance(update, rpc.CommutatorUpdate)
                    updates.append(update)
            return True

        monitoring_task = asyncio.create_task(monitoring())
        self.assertTrue(await utils.wait_for(lambda: monitoring_started))

        # spawn a ship and check, that an update is received
        for i in range(10):
            status, spawned_ship = await self.administrator.spawner.spawn_ship(
                player="player",
                blueprint="Ship/Miner",
                ship_name=f"Miner_#{i}",
                position=randomizer.random_position(
                    center=types.Position(0, 0, velocity=types.Vector(0, 0)),
                    radius=100000
                )
            )
            self.assertTrue(status.is_success())
            self.assertIsNotNone(spawned_ship)
            assert isinstance(spawned_ship, types.PhysicalObject)

            # Wait for update with spawned ship
            update = await wait_for_update()
            self.assertIsNotNone(update)
            self.assertIsNotNone(update.module_attached)
            self.assertEqual(update.module_attached.type, "Ship/Miner")
            self.assertEqual(update.module_attached.name, f"Miner_#{i}")

            # Check that commutator has added a ship and check ship's
            # position
            ship = modules.Ship.get_ship_by_name(
                commutator=commutator,
                name=update.module_attached.name
            )
            self.assertIsNotNone(ship)
            position = await ship.get_position()
            self.assertIsNotNone(position)
            types.Position.almost_equal(
                spawned_ship.position.predict(position.timestamp.usec()),
                position
            )

        # Cancel monitoring task
        monitoring_task.cancel()
        await asyncio.wait_for(monitoring_task, timeout=1)
        self.assertTrue(monitoring_task.done())
        self.assertTrue(monitoring_task.result())
