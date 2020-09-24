from typing import Optional
import asyncio

from tests.base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world

from server.configurator.modules import (
    default_ships,
    ResourceContainerState,
    ShipType
)
from server.configurator import Configuration, General, ApplicationMode

import expansion.procedures as procedures
from expansion.types import ResourceType, ResourceItem
from expansion.interfaces.public import ResourceContainer


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

        commutator, error = await self.login('player')
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        miner_1 = await procedures.connect_to_ship(ShipType.MINER.value, "miner-1", commutator)
        self.assertIsNotNone(miner_1)

        cargo = await procedures.connect_to_resource_container(name='cargo', ship=miner_1)
        self.assertIsNotNone(cargo)

        content = await cargo.get_content()
        self.assertAlmostEqual(20000, content.resources[ResourceType.e_SILICATES])
        self.assertAlmostEqual(50000, content.resources[ResourceType.e_METALS])
        self.assertAlmostEqual(15000, content.resources[ResourceType.e_ICE])

    @BaseTestFixture.run_as_sync
    async def test_open_close_port(self):
        await self.system_clock_fast_forward(speed_multiplier=25)

        commutator, error = await self.login('player')
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        miner_1 = await procedures.connect_to_ship(ShipType.MINER.value, "miner-1", commutator)
        self.assertIsNotNone(miner_1)

        cargo: Optional[ResourceContainer] =\
            await procedures.connect_to_resource_container(name='cargo', ship=miner_1)
        self.assertIsNotNone(cargo)

        self.assertIsNone(cargo.get_opened_port())

        # Trying to close a port, that has not been opened
        status = await cargo.close_port()
        self.assertEqual(ResourceContainer.Status.PORT_IS_NOT_OPENED, status)

        # Opening a port
        access_key = 12456
        status, port = await cargo.open_port(access_key=access_key)
        self.assertTrue(status.is_ok())
        self.assertNotEqual(0, port)

        self.assertEqual(port, cargo.get_opened_port()[0])
        self.assertEqual(access_key, cargo.get_opened_port()[1])

        # Trying to open yet another port
        status, port = await cargo.open_port(access_key=access_key*2)
        self.assertEqual(ResourceContainer.Status.PORT_ALREADY_OPEN, status)
        self.assertEqual(0, port)

        # Closing port (twice)
        status = await cargo.close_port()
        self.assertEqual(ResourceContainer.Status.SUCCESS, status)
        status = await cargo.close_port()
        self.assertEqual(ResourceContainer.Status.PORT_IS_NOT_OPENED, status)

        # Opening a port again
        status, port = await cargo.open_port(access_key=access_key)
        self.assertTrue(status.is_ok())
        self.assertNotEqual(0, port)

    @BaseTestFixture.run_as_sync
    async def test_transfer_success_case(self):
        await self.system_clock_fast_forward(speed_multiplier=25)

        commutator, error = await self.login('player')
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        miner_1 = await procedures.connect_to_ship(ShipType.MINER.value, "miner-1", commutator)
        self.assertIsNotNone(miner_1)

        miner_1_cargo: Optional[ResourceContainer] =\
            await procedures.connect_to_resource_container(name='cargo', ship=miner_1)
        self.assertIsNotNone(miner_1_cargo)

        miner_2 = await procedures.connect_to_ship(ShipType.MINER.value, "miner-2", commutator)
        self.assertIsNotNone(miner_1)

        miner_2_cargo: Optional[ResourceContainer] = \
            await procedures.connect_to_resource_container(name='cargo', ship=miner_2)
        self.assertIsNotNone(miner_1_cargo)

        # Opening a port on miner_2
        access_key = 12456
        status, port = await miner_2_cargo.open_port(access_key=access_key)
        self.assertTrue(status.is_ok())
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
        self.assertEqual(ResourceContainer.Status.SUCCESS, status)
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

        commutator, error = await self.login('player')
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        miner_1 = await procedures.connect_to_ship(ShipType.MINER.value, "miner-1", commutator)
        self.assertIsNotNone(miner_1)

        miner_1_cargo: Optional[ResourceContainer] = \
            await procedures.connect_to_resource_container(name='cargo', ship=miner_1)
        self.assertIsNotNone(miner_1_cargo)

        miner_2 = await procedures.connect_to_ship(ShipType.MINER.value, "miner-2", commutator)
        self.assertIsNotNone(miner_1)

        miner_2_cargo: Optional[ResourceContainer] = \
            await procedures.connect_to_resource_container(name='cargo', ship=miner_2)
        self.assertIsNotNone(miner_1_cargo)

        # Port is not opened error:
        status = await miner_1_cargo.transfer(port=4, access_key=123456,
                                              resource=ResourceItem(ResourceType.e_METALS, 30000))
        self.assertEqual(ResourceContainer.Status.PORT_IS_NOT_OPENED, status)

        # Opening a port on miner_2
        access_key = 12456
        status, port = await miner_2_cargo.open_port(access_key=access_key)
        self.assertTrue(status.is_ok())
        self.assertNotEqual(0, port)

        # Invalid access key
        status = await miner_1_cargo.transfer(port=port, access_key=access_key-1,
                                              resource=ResourceItem(ResourceType.e_METALS, 30000))
        self.assertEqual(ResourceContainer.Status.INVALID_ACCESS_KEY, status)
