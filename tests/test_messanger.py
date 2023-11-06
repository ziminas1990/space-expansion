from typing import Tuple, Optional, List
import random
import asyncio

from base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world

from server.configurator.configuration import Configuration
from server.configurator.general import General, ApplicationMode
from server.configurator.resources import ResourcesList

from expansion.modules import Messanger, MessangerStatus
from expansion.procedures import Connection
from expansion import modules
from expansion import types

class Session():
    def __init__(self, connection: Connection,
                 commutator: modules.Commutator,
                 messanger: modules.Messanger):
        self.connection: Connection = connection
        self.commutator: modules.Commutator = commutator
        self.messanger: modules.Messanger = messanger


class ReverseService:
    def __init__(self, service: modules.Messanger.Service) -> None:
        self.service = service

    async def run(self):
        self.service.flush()
        status, request = await self.service.wait_request()
        self.service
        while status.is_ok():
            await self.service.send_response(request, request.body[::-1])
            status, request = await self.service.wait_request()
            if status == types.WellKnownId.CANCELLED:
                return


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
                    password="awesome",
                    ships=[]
                )
            },
            world=world.World(),
        )

    def get_configuration(self) -> Configuration:
        return self.configuration

    async def open_new_session(self, player: str) -> Tuple[types.Status, Optional[Session]]:
        connection, error = await self.login(player, "127.0.0.1")
        if error:
            return types.Status.fail(f"Failed to login: {error}"), None
        assert(connection)
        commutator = connection.commutator
        messanger = Messanger.get_module(commutator)
        if not messanger:
            return types.Status.fail(f"Can't get messanger module!"), None
        return types.Status.ok(), Session(connection, commutator, messanger)


    @BaseTestFixture.run_as_sync
    async def _test_open_service(self):
        await self.system_clock_fast_forward(speed_multiplier=20)

        status, session = await self.open_new_session("player")
        self.assertTrue(session, str(status))

        status, service = await session.messanger.open_service("awesomesvc")
        self.assertTrue(status.is_ok(), str(status))
        self.assertIsNotNone(service)

    @BaseTestFixture.run_as_sync
    async def _test_open_service_fails(self):
        await self.system_clock_fast_forward(speed_multiplier=20)

        status, session = await self.open_new_session("player")
        self.assertTrue(session, str(status))

        # Try to create service with the same name
        status, service = await session.messanger.open_service("awesomesvc")
        self.assertTrue(status.is_ok(), str(status))
        status, _ = await session.messanger.open_service("awesomesvc")
        self.assertEqual(str(status), "can't open sevice: service exists")
        await service.close();

        # Try to esceed servers limit
        limit = 32  # value is hardcoded in server's sources
        for i in range(0, 32):
            status, _ = await session.messanger.open_service(f"svc_${i}")
            self.assertTrue(status.is_ok())
        status, service = await session.messanger.open_service(f"yet_another_service")
        self.assertEqual(str(status), "can't open sevice: too many services")
        self.assertIsNone(service)

    @BaseTestFixture.run_as_sync
    async def _test_services_list(self):
        await self.system_clock_fast_forward(speed_multiplier=20)

        status, session = await self.open_new_session("player")
        self.assertTrue(session, str(status))

        all_services = [f"service_{i}" for i in range(0, 32)]
        random.shuffle(all_services)

        services: List[Messanger.Service] = []

        # Add services in random order and check that they are in services list
        for service_name in all_services:
            status, service = await session.messanger.open_service(service_name)
            self.assertTrue(status.is_ok(), str(status))
            assert service is not None
            services.append(service)

            # Check that service is in services list
            status, opened_services = await session.messanger.serivces_list()
            self.assertTrue(status.is_ok(), str(status))

            self.assertEqual(
                sorted(opened_services),
                sorted([service.name for service in services])
            )

        # Remove services in random order
        random.shuffle(services)
        while len(services) > 0:
            service = services.pop()
            await service.close()

            # Check services list
            status, opened_services = await session.messanger.serivces_list()
            self.assertTrue(status.is_ok(), str(status))

            # a = sorted(opened_services)
            # b = sorted([service.name for service in services])
            self.assertEqual(
                sorted(opened_services),
                sorted([service.name for service in services])
            )

    @BaseTestFixture.run_as_sync
    async def _test_send_message_fails(self):
        max_request_timeout = 5
        sessions_limit = 256

        await self.system_clock_fast_forward(speed_multiplier=20)

        status, session = await self.open_new_session("player")
        self.assertTrue(session, str(status))

        status, response = await session.messanger.send_request("wrongsvc", "ping")
        self.assertTrue(status == MessangerStatus.NO_SUCH_SERVICE)

        service_name = "awesomesvc"
        status, service = await session.messanger.open_service(service_name=service_name)
        self.assertTrue(status, str(status))

        status, response = await session.messanger.send_request(
            service=service_name,
            request="ping",
            timeout=max_request_timeout + 0.01)
        self.assertTrue(status == MessangerStatus.REQUEST_TIMEOUT_TOO_LONG)

        timeout_s = 2
        status, response = await session.messanger.send_request(
            service=service_name,
            request="ping",
            timeout=timeout_s)
        # Service is not going to handle request, so request should fail
        # by timeout
        self.assertTrue(status == MessangerStatus.CLOSED)


    @BaseTestFixture.run_as_sync
    async def test_send_simple_ping(self):
        max_request_timeout = 5000
        sessions_limit = 256

        await self.system_clock_fast_forward(speed_multiplier=20)

        status, session = await self.open_new_session("player")
        self.assertTrue(session, str(status))

        status, response = await session.messanger.send_request("wrongsvc", "ping")
        self.assertTrue(status == MessangerStatus.NO_SUCH_SERVICE)

        service_name = "awesomesvc"
        status, service = await session.messanger.open_service(service_name=service_name)
        self.assertTrue(status, str(status))

        reverse_service = ReverseService(service=service)

        # Send request but get no response (service logic is not running)
        status, response = await session.messanger.send_request(
            service=service_name,
            request="ping")
        self.assertTrue(status == MessangerStatus.CLOSED)

        # Run service's logic
        reverse_service_task = asyncio.create_task(reverse_service.run())

        # Send request and get an answer
        request_body = "djksjfkdsljf"
        status, response = await session.messanger.send_request(
            service=service_name,
            request=request_body,
            timeout=5)
        self.assertTrue(status.is_ok())
        self.assertEqual(response, request_body[::-1])

        reverse_service_task.cancel()