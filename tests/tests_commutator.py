import asyncio
from typing import List, Optional
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

        # 1. player logins
        connection, error = await self.login('player', "127.0.0.1")
        self.assertIsNotNone(connection)
        commutator = connection.commutator
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)
        self.assertEqual(modules.Ship.get_all_ships(commutator), [])

        # 2. start commutator monitoring
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

        # 3. spawn ships and check they appear on the player
        for i in range(10):
            ship_name = f"Miner_#{i}"

            # 3.1 administrator spawns a ship
            status, spawned_ship = await self.administrator.spawner.spawn_ship(
                player="player",
                blueprint="Ship/Miner",
                ship_name=ship_name,
                position=randomizer.random_position(
                    center=types.Position(0, 0, velocity=types.Vector(0, 0)),
                    radius=100000
                )
            )
            self.assertTrue(status.is_success())
            self.assertIsNotNone(spawned_ship)
            assert isinstance(spawned_ship, types.PhysicalObject)

            # 3.2 wait for the ship attached update
            update = await wait_for_update()
            self.assertIsNotNone(update)
            self.assertIsNotNone(update.module_attached)
            self.assertEqual(update.module_attached.type, modules.ModuleType.SHIP.value)
            self.assertEqual(update.module_attached.name, ship_name)
            self.assertEqual(update.module_attached.blueprint_name, "Ship/Miner")

            # 3.3 look up the ship in the commutator registry
            ship = modules.Ship.get_ship_by_name(
                commutator=commutator,
                name=update.module_attached.name
            )
            self.assertIsNotNone(ship)
            self.assertEqual(ship.type, modules.ModuleType.SHIP.value)
            self.assertEqual(ship.ship_class, "Ship/Miner")

            # 3.4 check predicted position matches
            position = await ship.get_position()
            self.assertIsNotNone(position)
            types.Position.almost_equal(
                spawned_ship.position.predict(position.timestamp.usec()),
                position
            )

        # 4. cancel monitoring
        monitoring_task.cancel()
        await asyncio.wait_for(monitoring_task, timeout=1)
        self.assertTrue(monitoring_task.done())
        self.assertTrue(monitoring_task.result())

    @BaseTestFixture.run_as_sync
    async def test_spawned_ship_reports_fixed_type_and_blueprint(self):
        # 1. player logins
        connection, error = await self.login('player', "127.0.0.1")
        self.assertIsNotNone(connection)
        commutator = connection.commutator
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        # 2. administrator spawns a ship without the client opening a
        #    tunnel to it first
        status, spawned_ship = await self.administrator.spawner.spawn_ship(
            player="player",
            blueprint="Ship/Miner",
            ship_name="Miner-1",
            position=types.Position(0, 0, velocity=types.Vector(0, 0))
        )
        self.assertTrue(status.is_success())
        self.assertIsNotNone(spawned_ship)

        # 3. wait until the ship is listed on the root commutator
        self.assertTrue(await commutator.update())
        self.assertTrue(await utils.wait_for(
            lambda: modules.get_ship(commutator, "Miner-1") is not None
        ))

        # 4. read module info from the commutator (no ship specification)
        session = await commutator.open_session(rpc.CommutatorI)
        self.assertIsNotNone(session)
        assert isinstance(session, rpc.CommutatorI)
        infos = await session.get_all_modules()
        self.assertIsNotNone(infos)
        ship_info = next(
            (info for info in infos if info.name == "Miner-1"),
            None
        )
        self.assertIsNotNone(ship_info)
        self.assertEqual(ship_info.type, modules.ModuleType.SHIP.value)
        self.assertEqual(ship_info.name, "Miner-1")
        self.assertEqual(ship_info.blueprint_name, "Ship/Miner")

        # 5. the highlevel ship uses the same blueprint as ship_class
        ship = modules.get_ship(commutator, "Miner-1")
        self.assertIsNotNone(ship)
        self.assertEqual(ship.type, modules.ModuleType.SHIP.value)
        self.assertEqual(ship.ship_class, "Ship/Miner")
