from typing import Tuple, Optional, List, NamedTuple, Callable
import random
import asyncio
from enum import Enum
import string

from base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world

from server.configurator.configuration import Configuration
from server.configurator.general import General, ApplicationMode

from expansion.modules import Messanger, MessangerStatus, SystemClock
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

    def cut_by_now(self, now: int) -> "Interval":
        return Interval(
            begin=now if now > self.begin else self.begin,
            end=now if now > self.end else self.end
        )

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
                 name: str,
                 multiplier: int,
                 lifetime: Interval) -> None:
        self.session = session
        self.name = name
        self.service: Optional[modules.Messanger.Service] = None
        self.multiplies = multiplier
        self.lifetime = lifetime
        self.registered = False
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

    async def __impl(self):
        await self.session.clock.wait_until(self.lifetime.begin)

        status, self.service = await self.session.messanger.open_service(self.name)
        if not status:
            assert status, str(status)
            return
        self.registered = True

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
        self.registered = False

    def expected_response(self, request: str) -> str:
        return request * self.multiplies

    def is_active(self, now: int, margin: float = 0.05):
        return self.lifetime.with_margin(margin).contains(now) and self.registered

    def is_inactive(self, now: int, margin: float = 0.05):
        return not self.lifetime.contains(now) and not self.registered


class Environment:
    class Status:
        SERVICE_STILL_ACTIVE = "Environment.Status.SERVICE_STILL_ACTIVE"

    next_service_id = 1

    def __init__(self, lifetime: Interval):
        self.services: List[DuplicatingService] = []
        self.lifetime = lifetime

    def get_active_services(self, now: int) -> List[DuplicatingService]:
        services: List[DuplicatingService] = []
        for service in self.services:
            if service.is_active(now):
                services.append(service)
        return services

    def get_non_active_services(self, now: int) -> List[DuplicatingService]:
        services: List[DuplicatingService] = []
        for service in self.services:
            if service.is_inactive(now) and service.done():
                services.append(service)
        return services

    # Spawn a new service with lifetime inside the specified frame
    def spawn_random_service(self, session: Session, lifetime: Interval) \
        -> Tuple[types.Status, Optional[DuplicatingService]]:
        # lifetime = self.lifetime.with_margin(0.05).random_sub_interval(
        #     length_part=0.1 + 0.4 * random.random())

        service_name = f"service_{Environment.next_service_id}"
        Environment.next_service_id += 1

        multiplier = random.randint(2, 20)
        duplicating_service = DuplicatingService(
            session=session,
            name=service_name,
            multiplier=multiplier,
            lifetime=lifetime)
        duplicating_service.run()
        self.services.append(duplicating_service)
        return types.Status.ok(), duplicating_service

    def check_is_done(self) -> types.Status:
        for service in self.services:
            if not service.done():
                return types.Status.fail(
                    what=f"Service {service.name} is still active",
                    status_id=Environment.Status.SERVICE_STILL_ACTIVE
                )
        return types.Status.ok()


class Dice(Enum):
    SPAWN_SERVICE = 1
    CHECK_SERVICES_LIST = 5
    REQUEST_TO_INACTIVE_SERVICE = 10
    SEND_REQUEST = 100

    @staticmethod
    def throw() -> "Dice":
        return random.choices(
            [action for action in Dice],
            [action.value for action in Dice])[0]


class Player:
    WRONG_RESPONSE="Player.WRONG_RESPONSE"
    ACCESS_TO_INACTIVE_SERVICE="Player.ACCESS_TO_INACTIVE_SERVICE"
    SERVICES_LIST_MISMATCH="Player.SERVICES_LIST_MISMATCH"

    def __init__(self, session: Session, env: Environment,
                 lifetime: Interval) -> None:
        self.session = session
        self.env = env
        self.lifetime = lifetime
        self.services: List[DuplicatingService] = []

    async def do_something(self) -> types.Status:
        now = await self.session.clock.time()
        if not self.lifetime.contains(now):
            # Just do nothing
            return types.Status.ok()

        action = Dice.throw()
        if action == Dice.SEND_REQUEST:
            return await self.send_random_request(now)
        if action == Dice.SPAWN_SERVICE:
            status, service = self.env.spawn_random_service(
                session=self.session,
                lifetime=self.lifetime
                    .cut_by_now(now)
                    .with_margin(0.05)
                    .random_sub_interval(0.3 + 0.5 * random.random())
            )
            if service is not None:
                self.services.append(service)

            if status == MessangerStatus.TOO_MANY_SERVCES:
                if len(self.env.get_active_services(now)) == 32:
                    # Expected behavior
                    return types.Status.ok()
            return status
        if action == Dice.REQUEST_TO_INACTIVE_SERVICE:
            return await self.send_random_request_to_inactive_service(now)
        if action == Dice.CHECK_SERVICES_LIST:
            return await self.check_services_list()
        return types.Status.ok()

    # Send a request to any random service, that should be active at the
    # specified 'now'
    async def send_random_request(self, now: int) -> types.Status:
        candidates = self.env.get_active_services(now)
        if len(candidates) == 0:
            return types.Status.ok()
        service = random.choice(candidates)
        random_request = "".join(random.choice(string.ascii_letters) for _ in range(8))
        status, response = await self.session.messanger.send_request(
            service=service.name,
            request=random_request,
            timeout=3
        )

        if not status.is_ok():
            return status.wrap_fail(
                f"failed to send request to {service.name}")

        expected_response = service.expected_response(random_request)
        if response != expected_response:
            return types.Status.fail(
                what= " ".join([
                    f"Got unexpected response: '{response}'.",
                    f"Expected: {expected_response}"]),
                status_id=Player.WRONG_RESPONSE)
        return types.Status.ok()

    # Send a request to any random service, that should be inactive at the
    # specified 'now'
    async def send_random_request_to_inactive_service(self, now: int) -> types.Status:
        candidates = self.env.get_non_active_services(now)
        if len(candidates) == 0:
            return types.Status.ok()
        service = random.choice(candidates)
        random_request = "test"
        status, response = await self.session.messanger.send_request(
            service=service.name,
            request=random_request,
            timeout=0.5
        )

        if status == MessangerStatus.NO_SUCH_SERVICE:
            return types.Status.ok()
        return types.Status.fail(
            f"Got a response from inactive service {service.name}",
            status_id=Player.ACCESS_TO_INACTIVE_SERVICE
        )

    async def check_services_list(self) -> types.Status:
        status, services, timestamp = await self.session.messanger.serivces_list()
        if not status:
            return status.wrap_fail("Can't get services list")
        active_services = self.env.get_active_services(timestamp)
        for expected_service in active_services:
            if expected_service.name not in services:
                return types.Status.fail(
                    what=f"Services list mismatch! Service {expected_service.name} not listed",
                    status_id=Player.SERVICES_LIST_MISMATCH
                )
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
    async def test_single_player(self):
        await self.system_clock_fast_forward(speed_multiplier=20)

        status, main_session = await self.new_session()
        self.assertTrue(status.is_ok, str(status))

        started_at = await main_session.clock.time()
        # 2 minutes of ingame time should be enough for this test
        lifetime = Interval(
            begin=started_at + 10 * 1000 * 1000,
            end=started_at + 130 * 1000 * 1000)
        assert(started_at is not None)

        environment = Environment(lifetime)

        # Spawn a number of services
        services_total = 3
        services: List[DuplicatingService] = []
        for _ in range(services_total):
            status, service = environment.spawn_random_service(
                session=main_session,
                lifetime=environment.lifetime.with_margin(0.05))
            self.assertTrue(status, str(status))
            services.append(service)

        status, session = await self.new_session()
        self.assertTrue(status.is_ok, str(status))
        player = Player(session, environment, environment.lifetime.with_margin(0.01))

        # Do some random thing each ~200ms
        now = await main_session.clock.time()
        while now < lifetime.end:
            status = await player.do_something()
            self.assertTrue(status, str(status))
            now = await main_session.clock.wait_for(200000)

        # Give to services some time to finish
        await asyncio.sleep(0.2)

        status = environment.check_is_done()
        self.assertTrue(status, str(status))

    @BaseTestFixture.run_as_sync
    async def test_multiple_players(self):
        await self.system_clock_fast_forward(speed_multiplier=20)

        status, main_session = await self.new_session()
        self.assertTrue(status.is_ok, str(status))

        started_at = await main_session.clock.time()
        # 2 minutes of ingame time should be enough for this test
        lifetime = Interval(
            begin=started_at + 10 * 1000 * 1000,
            end=started_at + 130 * 1000 * 1000)
        assert(started_at is not None)

        environment = Environment(lifetime)

        # Spawn a number of services
        services_total = 3
        services: List[DuplicatingService] = []
        for _ in range(services_total):
            status, service = environment.spawn_random_service(
                session=main_session,
                lifetime=environment.lifetime.with_margin(0.05))
            self.assertTrue(status, str(status))
            services.append(service)

        # Spawn a number of player instances (the same player but different
        # connections)
        players: List[Player] = []
        for _ in range(15):
            status, session = await self.new_session()
            self.assertTrue(status.is_ok, str(status))
            player = Player(
                session=session,
                env=environment,
                lifetime=environment.lifetime
                    .with_margin(0.01)
                    .random_sub_interval(0.5 + 0.4 * random.random()))
            players.append(player)

        # Do some random thing each ~100ms
        now = await main_session.clock.time()
        while now < lifetime.end:
            for player in players:
                status = await player.do_something()
                self.assertTrue(status, str(status))
            now = await main_session.clock.wait_for(100000)

        # Give to services some time to finish
        await asyncio.sleep(0.2)

        status = environment.check_is_done()
        self.assertTrue(status, str(status))