from typing import Optional, NamedTuple, List, Tuple
from enum import Enum

import expansion.api as api
from expansion.transport import IOTerminal, Channel
import expansion.utils as utils
import expansion.types as types


class MessangerStatus:
    ROUTED = "MessangerI.ROUTED"
    SERVICE_EXISTS = "MessangerI.SERVICE_EXISTS"
    NO_SUCH_SERVICE = "MessangerI.NO_SUCH_SERVICE"
    TOO_MANY_SERVCES = "MessangerI.TOO_MANY_SERVCES"
    SESSION_BUSY = "MessangerI.SESSION_BUSY"
    WRONG_SEQ = "MessangerI.WRONG_SEQ"
    CLOSED = "MessangerI.CLOSED"
    REQUEST_TIMEOUT_TOO_LONG = "MessangerI.REQUEST_TIMEOUT_TOO_LONG"
    SESSIONS_LIMIT_REACHED = "MessangerI.SESSIONS_LIMIT_REACHED"


class MessangerI(IOTerminal):

    @staticmethod
    def make_status(status: api.IMessanger.Status) -> types.Status:
        ProtobufStatus = api.IMessanger.Status
        if status == ProtobufStatus.SUCCESS:
            return types.Status.ok()
        elif status == ProtobufStatus.ROUTED:
            return types.Status.ok("routed", MessangerStatus.ROUTED)
        elif status == ProtobufStatus.SERVICE_EXISTS:
            return types.Status.fail("service exists", MessangerStatus.SERVICE_EXISTS)
        elif status == ProtobufStatus.NO_SUCH_SERVICE:
            return types.Status.fail("no such service", MessangerStatus.NO_SUCH_SERVICE)
        elif status == ProtobufStatus.TOO_MANY_SERVCES:
            return types.Status.fail("too many services", MessangerStatus.TOO_MANY_SERVCES)
        elif status == ProtobufStatus.SESSION_BUSY:
            return types.Status.fail("session busy", MessangerStatus.SESSION_BUSY)
        elif status == ProtobufStatus.WRONG_SEQ:
            return types.Status.fail("wrong sequence", MessangerStatus.WRONG_SEQ)
        elif status == ProtobufStatus.CLOSED:
            return types.Status.fail("session closed", MessangerStatus.CLOSED)
        elif status == ProtobufStatus.UNKNOWN_ERROR:
            return types.Status.unknown()
        elif status == ProtobufStatus.REQUEST_TIMEOUT_TOO_LONG:
            return types.Status.fail("request timeout is too long", MessangerStatus.REQUEST_TIMEOUT_TOO_LONG)
        elif status == ProtobufStatus.SESSIONS_LIMIT_REACHED:
            return types.Status.fail("sessions limit reqched", MessangerStatus.SESSIONS_LIMIT_REACHED)
        else:
            return types.Status.unknown()


    @staticmethod
    def is_routed(status: types.Status) -> bool:
        return status == MessangerStatus.ROUTED

    class Request(NamedTuple):
        service: str
        seq: int
        timeout_ms: int
        body: str

    def __init__(self, name: Optional[str] = None):
        super().__init__(name=name or utils.generate_name(MessangerI))

    @Channel.return_on_close(types.Status.channel_is_closed())
    async def open_service(self, service_name: str, force: bool, timeout: float = 0.5) -> types.Status:
        request = api.Message()
        body = request.messanger.open_service
        body.service_name = service_name
        body.force = force
        if not self.send(message=request):
            return types.Status.failed_to_send(request)

        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return types.Status.response_timeout()
        status = api.get_message_field(response, ["messanger", "open_service_status"])
        if status is None:
            return types.Status.unexpected_message(response)
        return MessangerI.make_status(status)

    @Channel.return_on_cancel(types.Status.cancelled(), None)
    @Channel.return_on_close(types.Status.channel_is_closed(), None)
    async def wait_request(self, timeout: float = 0) -> Tuple[types.Status, Optional[Request]]:
        message, _ = await self.wait_message(timeout=timeout)
        if not message:
            # Just timeout has expired
            return types.Status.timeout(), None
        request = api.get_message_field(message, ["messanger", "request"])
        if request is None:
            return types.Status.unexpected_message(message), None
        return types.Status.ok(), MessangerI.Request(
            service=request.service,
            seq=request.seq,
            timeout_ms=request.timeout_ms,
            body=request.body),

    @Channel.return_on_close(types.Status.channel_is_closed(), [])
    async def services_list(self, timeout: float = 0.5) -> Tuple[types.Status, List[str]]:
        request = api.Message()
        request.messanger.services_list_req = True
        if not self.send(message=request):
            return types.Status.failed_to_send(request), []

        services: List[str] = []
        finished = False
        while not finished:
            response, _ = await self.wait_message(timeout=timeout)
            if not response:
                return types.Status.timeout(), services
            body = api.get_message_field(response, ["messanger", "services_list"])
            if body is None:
                return types.Status.unexpected_message(response), services
            services.extend(body.services)
            finished = body.left == 0

        return types.Status.ok(), services

    @Channel.return_on_close(types.Status.channel_is_closed())
    async def send_request(self, service: str, seq: int, body: str, timeout: float = 1) \
        -> types.Status:
        message = api.Message()
        request = message.messanger.request
        request.service = service
        request.seq = seq
        request.body = body
        request.timeout_ms = round(timeout * 1000)
        if not self.send(message=message):
            return types.Status.failed_to_send(message)
        return types.Status.ok()

    @Channel.return_on_close(types.Status.channel_is_closed())
    async def send_response(self, request: Request, body: str, timeout: float = 1) \
        -> types.Status:
        message = api.Message()
        response = message.messanger.response
        response.seq = request.seq
        response.body = body
        if not self.send(message=message):
            return types.Status.failed_to_send(message)
        return types.Status.ok()

    @Channel.return_on_close(types.Status.channel_is_closed())
    async def wait_session_status(self, timeout: float = 0.5) -> Tuple[types.Status, Optional[int]]:
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return types.Status.timeout(), None
        body = api.get_message_field(response, ["messanger", "session_status"])
        if body is None:
            return types.Status.unexpected_message(response), None
        return MessangerI.make_status(body.status), body.seq

    @Channel.return_on_close(types.Status.channel_is_closed())
    async def wait_response(self, timeout: float = 1) \
        -> Tuple[types.Status, Optional[int], Optional[str]]:
        message, _ = await self.wait_message(timeout=timeout)
        if not message:
            return types.Status.timeout(), None, None
        response = api.get_message_field(message, ["messanger", "response"])
        if response is None:
            session_status = api.get_message_field(message, ["messanger", "session_status"])
            if session_status is not None:
                return MessangerI.make_status(session_status.status), session_status.seq, None
            return types.Status.unexpected_message(response), session_status.seq, None
        return types.Status.ok(), response.seq, response.body