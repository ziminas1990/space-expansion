from typing import Tuple, Optional, List, NamedTuple, Callable
import random
import asyncio
import abc
import string

from base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world

from server.configurator.configuration import Configuration
from server.configurator.general import General, ApplicationMode

from expansion.modules import Messanger, SystemClock
from expansion.procedures import Connection
from expansion import modules
from expansion import types


class Interval(NamedTuple):
    begin: int
    end: int

    def contains(self, point: int) -> bool:
        return self.begin <= point <= self.end

    def length(self) -> int:
        return self.end - self.begin

    def with_margin(self, margin: float) -> "Interval":
        assert(0 < 2 * margin < 1)
        margin_us = round((self.end - self.begin) * margin)
        return Interval(begin=self.begin + margin_us, end=self.end - margin_us)

    def random_sub_interval(self, length_part: float) -> "Interval":
        assert(0 < length_part < 1)
        frame_length = self.end - self.begin
        length = round(frame_length * length_part)

        left_offset = round(random.randint(0, frame_length - length))
        return Interval(
            begin=self.begin + left_offset,
            end=self.begin + left_offset + length)


class Session:
    def __init__(self,
                 connection: Connection,
                 commutator: modules.Commutator,
                 messanger: modules.Messanger,
                 clock: modules.SystemClock):
        self.connection = connection
        self.commutator = commutator
        self.messanger = messanger
        self.clock = clock


class DuplicatingService:
    def __init__(self,
                 session: Session,
                 service: modules.Messanger.Service,
                 multiplier: int,
                 lifetime: Interval) -> None:
        self.session = session
        self.service = service
        self.multiplies = multiplier
        self.lifetime = lifetime
        self.task: Optional[asyncio.Task] = None

    def run(self):
        if self.task is None:
            self.task = asyncio.create_task(self.__impl())

    def done(self) -> bool:
        return self.task is None or self.task.done()

    def interrupt(self):
        if self.task is not None:
            self.task.cancel()
            self.task = None

    def name(self) -> str:
        return self.service.name

    async def __impl(self):
        await self.session.clock.wait_until(self.lifetime.begin)

        self.service.flush()
        now = await self.session.clock.time()

        while now < self.lifetime.end:
            status, request = await self.service.wait_request(timeout=0.1)
            if status == types.Status.CANCELLED:
                break
            if status == types.Status.TIMEOUT:
                # It's okay, just no requests for now
                status = types.Status.ok()
            elif request is not None:
                await self.service.send_response(
                    request=request,
                    response=self.expected_response(request.body))
            now = await self.session.clock.time()

        # Service has run out of lifetime or has been interrupted
        await self.service.close()

    def expected_response(self, request: str) -> str:
        return request * self.multiplies

    def is_alive(self, now: int, margin: float = 0.05):
        return self.lifetime.with_margin(0.01).contains(now)


class Environment:
    next_service_id = 1

    class Status:
        WRONG_RESPONSE="Environment.Status.WRONG_RESPONSE"


    def __init__(self, lifetime: Interval):
        self.services: List[DuplicatingService] = []
        self.lifetime = lifetime

    def get_active_services(self, now: int) -> List[DuplicatingService]:
        services: List[DuplicatingService] = []
        for service in self.services:
            if service.is_alive(now):
                services.append(service)
        return services

    # Spawn a new service with lifetime inside the specified frame
    async def spawn_random_service(self, session: Session) \
        -> Tuple[types.Status, Optional[DuplicatingService]]:
        lifetime = self.lifetime.with_margin(0.05).random_sub_interval(
            length_part=0.1 + 0.4 * random.random())

        service_name = f"service_{Environment.next_service_id}"
        Environment.next_service_id += 1

        status, lowlevel_service = await session.messanger.open_service(
            service_name=service_name)
        if not status.is_ok():
            return status.wrap_fail("Can't spawn service"), None

        multiplier = random.randint(2, 20)
        duplicating_service = DuplicatingService(
            session=session,
            service=lowlevel_service,
            multiplier=multiplier,
            lifetime=lifetime)
        duplicating_service.run()
        self.services.append(duplicating_service)
        return types.Status.ok(), duplicating_service

    # Use the specified 'messanger' instance to send a request to any random
    # service, that is active at the specifie 'now'
    async def send_random_request(self, now: int, session: Session) -> types.Status:
        candidates = self.get_active_services(now)
        if len(candidates) == 0:
            return types.Status.ok()
        service = random.choice(candidates)
        random_request = "".join(random.choice(string.ascii_letters) for _ in range(8))
        status, response = await session.messanger.send_request(
            service=service.name(),
            request=random_request,
            timeout=3
        )

        if not status.is_ok():
            return status.wrap_fail(
                f"failed to send request to {service.name()}")

        expected_response = service.expected_response(random_request)
        if response != expected_response:
            return types.Status.fail(
                what= " ".join([
                    f"Got unexpected response: '{response}'.",
                    f"Expected: {expected_response}"]),
                status_id=Environment.Status.WRONG_RESPONSE)
        return types.Status.ok()


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

    async def new_session(self) -> Tuple[types.Status, Optional[Session]]:
        connection, error = await self.login("player", "127.0.0.1")
        if error:
            return types.Status.fail(f"Failed to login: {error}"), None
        assert(connection)
        commutator = connection.commutator
        clock = SystemClock.find(commutator)
        if not clock:
            return types.Status.fail(f"Can't get clock module!"), None
        await clock.initial_sync()

        messanger = Messanger.get_module(commutator)
        if not messanger:
            return types.Status.fail(f"Can't get messanger module!"), None

        return types.Status.ok(), Session(
            connection=connection,
            commutator=commutator,
            messanger=messanger,
            clock=clock)

    @BaseTestFixture.run_as_sync
    async def test_single_player_test(self):
        await self.system_clock_fast_forward(speed_multiplier=20)

        status, main_session = await self.new_session()
        self.assertTrue(status.is_ok, str(status))

        started_at = await main_session.clock.time()
        # 1 minutes of ingame time should be enough for this test
        lifetime = Interval(
            begin=started_at + 10 * 1000 * 1000,
            end=started_at + 70 * 1000 * 1000)
        assert(started_at is not None)

        environment = Environment(lifetime)
        status, player = await self.new_session()
        self.assertTrue(status.is_ok, str(status))

        # Spawn a number of services
        services_total = 30
        services: List[DuplicatingService] = []
        for _ in range(services_total):
            status, service = await environment.spawn_random_service(player)
            self.assertTrue(status, str(status))
            services.append(service)

        # Send request to random service each ~200ms
        now = await main_session.clock.time()
        while now < lifetime.end:
            status = await environment.send_random_request(now, player)
            self.assertTrue(status, str(status))
            now = await main_session.clock.wait_for(200000)

        # Give to services some time to finish
        await asyncio.sleep(0.2)

        for service in services:
            self.assertTrue(service.done())

