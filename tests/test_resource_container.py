import asyncio
import logging
from typing import Optional, List

from base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world

from server.configurator.modules import (
    default_ships,
    ResourceContainerState,
    ShipType
)
from server.configurator.configuration import Configuration
from server.configurator.general import General, ApplicationMode

from expansion import modules
from expansion.types import ResourceType, ResourceItem
from expansion.interfaces.rpc import ResourceContainerI


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
                        default_ships.make_miner(
                            name="miner-1",
                            position=world.Position(
                                x=0, y=0, velocity=world.Vector(0, 0)),
                            cargo=ResourceContainerState(
                                content={
                                    ResourceType.e_SILICATES: 20000,
                                    ResourceType.e_METALS: 50000,
                                    ResourceType.e_ICE: 15000,
                                }
                            )
                        ),
                        default_ships.make_miner(
                            name="miner-2",
                            position=world.Position(
                                x=10, y=10, velocity=world.Vector(0, 0)),
                            cargo=ResourceContainerState(
                                content={
                                    ResourceType.e_SILICATES: 5000,
                                    ResourceType.e_METALS: 5000,
                                    ResourceType.e_ICE: 10000,
                                }
                            )
                        ),
                    ]
                )
            },
        )

    def get_configuration(self) -> Configuration:
        return self.configuration

    @BaseTestFixture.run_as_sync
    async def test_get_content(self):
        await self.system_clock_fast_forward(speed_multiplier=25)

        connection, error = await self.login('player', "127.0.0.1")
        self.assertIsNotNone(connection)
        commutator = connection.commutator
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        miner_1 = modules.get_ship(commutator, ShipType.MINER.value, "miner-1")
        self.assertIsNotNone(miner_1)

        cargo = modules.get_cargo(commutator=miner_1, name='cargo')
        self.assertIsNotNone(cargo)

        content = await cargo.get_content()
        self.assertAlmostEqual(20000, content.resources[ResourceType.e_SILICATES])
        self.assertAlmostEqual(50000, content.resources[ResourceType.e_METALS])
        self.assertAlmostEqual(15000, content.resources[ResourceType.e_ICE])

    @BaseTestFixture.run_as_sync
    async def test_open_close_port(self):
        await self.system_clock_fast_forward(speed_multiplier=25)

        connection, error = await self.login('player', "127.0.0.1")
        self.assertIsNotNone(connection)
        commutator = connection.commutator
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        miner_1 = modules.get_ship(commutator, ShipType.MINER.value, "miner-1")
        self.assertIsNotNone(miner_1)

        cargo = modules.get_cargo(commutator=miner_1, name='cargo')
        self.assertIsNotNone(cargo)

        self.assertIsNone(cargo.get_opened_port())

        # Trying to close a port, that has not been opened
        status = await cargo.close_port()
        self.assertEqual(ResourceContainerI.Status.PORT_IS_NOT_OPENED, status)

        # Opening a port
        access_key = 12456
        status, port = await cargo.open_port(access_key=access_key)
        self.assertTrue(status.is_success())
        self.assertNotEqual(0, port)

        self.assertEqual(port, cargo.get_opened_port()[0])
        self.assertEqual(access_key, cargo.get_opened_port()[1])

        # Trying to open yet another port
        status, port = await cargo.open_port(access_key=access_key*2)
        self.assertEqual(ResourceContainerI.Status.PORT_ALREADY_OPEN, status)
        self.assertEqual(0, port)

        # Closing port (twice)
        status = await cargo.close_port()
        self.assertEqual(ResourceContainerI.Status.SUCCESS, status)
        status = await cargo.close_port()
        self.assertEqual(ResourceContainerI.Status.PORT_IS_NOT_OPENED, status)

        # Opening a port again
        status, port = await cargo.open_port(access_key=access_key)
        self.assertTrue(status.is_success())
        self.assertNotEqual(0, port)

    @BaseTestFixture.run_as_sync
    async def test_transfer_success_case(self):
        await self.system_clock_fast_forward(speed_multiplier=25)

        connection, error = await self.login('player', "127.0.0.1")
        self.assertIsNotNone(connection)
        commutator = connection.commutator
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        miner_1 = modules.get_ship(commutator, ShipType.MINER.value, "miner-1")
        self.assertIsNotNone(miner_1)
        miner_1_cargo = modules.get_cargo(commutator=miner_1, name='cargo')
        self.assertIsNotNone(miner_1_cargo)

        miner_2 = modules.get_ship(commutator, ShipType.MINER.value, "miner-2")
        self.assertIsNotNone(miner_2)
        miner_2_cargo = modules.get_cargo(commutator=miner_2, name='cargo')
        self.assertIsNotNone(miner_2_cargo)

        # Opening a port on miner_2
        access_key = 12456
        status, port = await miner_2_cargo.open_port(access_key=access_key)
        self.assertTrue(status.is_success())
        self.assertNotEqual(0, port)

        total_transferred_amount: float = 0.0  # Will be accumulated in progress callback

        def cb_progress_report(item: ResourceItem):
            self.assertEqual(ResourceType.e_METALS, item.resource_type)
            nonlocal total_transferred_amount
            total_transferred_amount += item.amount

        # Transferring resources from miner_1 to miner_2
        status = await miner_1_cargo.transfer(port=port, access_key=access_key,
                                              progress_cb=cb_progress_report,
                                              resource=ResourceItem(ResourceType.e_METALS, 30000))
        self.assertEqual(ResourceContainerI.Status.SUCCESS, status)
        self.assertAlmostEqual(30000, total_transferred_amount)

        # Check resources in containers
        content = await miner_1_cargo.get_content()
        self.assertAlmostEqual(20000, content.resources[ResourceType.e_METALS])
        self.assertAlmostEqual(20000, content.resources[ResourceType.e_SILICATES])
        self.assertAlmostEqual(15000, content.resources[ResourceType.e_ICE])

        content = await miner_2_cargo.get_content()
        self.assertAlmostEqual(35000, content.resources[ResourceType.e_METALS])
        self.assertAlmostEqual(5000, content.resources[ResourceType.e_SILICATES])
        self.assertAlmostEqual(10000, content.resources[ResourceType.e_ICE])

    @BaseTestFixture.run_as_sync
    async def test_transfer_fails_cases(self):
        await self.system_clock_fast_forward(speed_multiplier=25)

        connection, error = await self.login('player', "127.0.0.1")
        self.assertIsNotNone(connection)
        commutator = connection.commutator
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        miner_1 = modules.get_ship(commutator, ShipType.MINER.value, "miner-1")
        self.assertIsNotNone(miner_1)
        miner_1_cargo = modules.get_cargo(commutator=miner_1, name='cargo')
        self.assertIsNotNone(miner_1_cargo)

        miner_2 = modules.get_ship(commutator, ShipType.MINER.value, "miner-2")
        self.assertIsNotNone(miner_2)
        miner_2_cargo = modules.get_cargo(commutator=miner_2, name='cargo')
        self.assertIsNotNone(miner_2_cargo)

        # Port is not opened error:
        status = await miner_1_cargo.transfer(port=4, access_key=123456,
                                              resource=ResourceItem(ResourceType.e_METALS, 30000))
        self.assertEqual(ResourceContainerI.Status.PORT_IS_NOT_OPENED, status)

        # Opening a port on miner_2
        access_key = 12456
        status, port = await miner_2_cargo.open_port(access_key=access_key)
        self.assertTrue(status.is_success())
        self.assertNotEqual(0, port)

        # Invalid access key
        status = await miner_1_cargo.transfer(port=port, access_key=access_key-1,
                                              resource=ResourceItem(ResourceType.e_METALS, 30000))
        self.assertEqual(ResourceContainerI.Status.INVALID_ACCESS_KEY, status)

    @BaseTestFixture.run_as_sync
    async def test_transfer_monitoring(self):
        await self.system_clock_fast_forward(speed_multiplier=10)

        connection, error = await self.login('player', "127.0.0.1")
        self.assertIsNotNone(connection)
        commutator = connection.commutator
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        miner_1 = modules.get_ship(commutator, ShipType.MINER.value, "miner-1")
        self.assertIsNotNone(miner_1)
        miner_1_cargo = modules.get_cargo(commutator=miner_1, name='cargo')
        self.assertIsNotNone(miner_1_cargo)

        miner_2 = modules.get_ship(commutator, ShipType.MINER.value, "miner-2")
        self.assertIsNotNone(miner_2)
        miner_2_cargo = modules.get_cargo(commutator=miner_2, name='cargo')
        self.assertIsNotNone(miner_2_cargo)

        # Opening a port on miner_2
        access_key = 12456
        status, port = await miner_2_cargo.open_port(access_key=access_key)
        self.assertTrue(status.is_success())
        self.assertNotEqual(0, port)

        # Will be collected during transferring
        transactions: List[ResourceItem] = []
        # Will be collected in monitoring tasks
        miner_1_cargo_journal: List[ResourceContainerI.Content] = []
        miner_2_cargo_journal: List[ResourceContainerI.Content] = []

        async def do_monitoring(
                container: modules.ResourceContainer,
                journal: List[ResourceContainerI.Content]) \
                -> ResourceContainerI.Status:
            async for status, content in container.monitor():
                if content:
                    journal.append(content)
                else:
                    return status
            return ResourceContainerI.Status.SUCCESS

        miner_1_cargo_monitoring = asyncio.create_task(
            do_monitoring(miner_1_cargo, miner_1_cargo_journal),
        )

        miner_2_cargo_monitoring = asyncio.create_task(
            do_monitoring(miner_2_cargo, miner_2_cargo_journal),
        )

        status = await miner_1_cargo.transfer(
            port=port,
            access_key=access_key,
            progress_cb=transactions.append,
            resource=ResourceItem(ResourceType.e_METALS, 30000))

        # Wait some additional time for monitoring events
        await asyncio.sleep(0.2)

        self.assertTrue(miner_1_cargo_monitoring.cancel())
        self.assertTrue(miner_2_cargo_monitoring.cancel())
        await asyncio.wait([miner_1_cargo_monitoring,
                            miner_2_cargo_monitoring])

        # Check that all data are consistent
        self.assertEqual(len(miner_1_cargo_journal), len(miner_2_cargo_journal))
        self.assertEqual(len(miner_1_cargo_journal), len(transactions) + 1)
        self.assertEqual(len(miner_2_cargo_journal), len(transactions) + 1)

        current_content = miner_1_cargo_journal[0]
        for updated_content, transaction in \
                zip(miner_1_cargo_journal[1:], transactions):
            resource_type = transaction.resource_type
            amount = transaction.amount
            self.assertAlmostEqual(
                updated_content.resources[resource_type],
                current_content.resources[resource_type] - amount)
            current_content = updated_content

        current_content = miner_2_cargo_journal[0]
        for updated_content, transaction in \
                zip(miner_2_cargo_journal[1:], transactions):
            resource_type = transaction.resource_type
            amount = transaction.amount
            self.assertAlmostEqual(
                updated_content.resources[resource_type],
                current_content.resources[resource_type] + amount)
            current_content = updated_content
