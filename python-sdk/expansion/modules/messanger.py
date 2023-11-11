from typing import Optional, Tuple, List, cast, TYPE_CHECKING

from expansion.interfaces.rpc import MessangerI, MessangerStatus
import expansion.utils as utils
from expansion import types
from .base_module import ModuleType

from .base_module import BaseModule, TunnelFactory

if TYPE_CHECKING:
    from expansion.modules import Commutator


class Messanger(BaseModule):

    class Service:
        def __init__(self, name: str, session: MessangerI):
            self.name = name
            self.session = session

        async def wait_request(self, timeout: float = 0):
            return await self.session.wait_request(timeout=timeout)

        async def send_response(self, request: MessangerI.Request, response: str):
            return await self.session.send_response(request=request, body=response)

        def flush(self):
            self.session.flush()

        async def close(self):
            await self.session.close()

    Request = MessangerI.Request

    def __init__(self,
                 tunnel_factory: TunnelFactory,
                 name: Optional[str] = None):
        super().__init__(tunnel_factory=tunnel_factory,
                         name=name or utils.generate_name(Messanger))
        self.next_seq: int = 1

    async def open_service(
            self,
            service_name: str,
            force: bool = False,
            timeout: float = 0.5) -> Tuple[types.Status, Optional[Service]]:

        session = await self.open_session(MessangerI)
        assert session is not None
        messanger = cast(MessangerI, session)

        status = await messanger.open_service(service_name, force, timeout)
        if not status:
            return status.wrap_fail("can't open sevice"), None

        return types.Status.ok(), Messanger.Service(name=service_name, session=messanger)

    @BaseModule.use_session(
        terminal_type=MessangerI,
        return_on_unreachable=([], types.Status.unreachable()),
        return_on_cancel=([], types.Status.cancelled()))
    async def serivces_list(
            self,
            timeout: float = 0.5,
            session: Optional[MessangerI] = None) -> Tuple[types.Status, List[str]]:
        assert session is not None
        return await session.services_list(timeout)

    @BaseModule.use_session(
        terminal_type=MessangerI,
        return_on_unreachable=([], types.Status.unreachable()),
        return_on_cancel=([], types.Status.cancelled()))
    async def send_request(
            self,
            service: str,
            request: str,
            timeout: float = 1,
            session: Optional[MessangerI] = None) -> Tuple[types.Status, Optional[str]]:
        assert session is not None
        seq = self.next_seq
        self.next_seq += 1
        status = await session.send_request(
            service=service, seq=seq, body=request, timeout=timeout)
        if not status:
            return status.wrap_fail("can't send request"), None

        status, status_seq = await session.wait_session_status()
        if status_seq != seq:
            return status.wrap_fail(f"got unexpected status sequence {status_seq}"), None
        if status != MessangerStatus.ROUTED:
            return status.wrap_fail("request has not been routed"), None

        status, response_seq, response = await session.wait_response(timeout=timeout)
        if not status:
            return status.wrap_fail("can't get response"), None
        if response_seq != seq:
            return status.wrap_fail(f"got unexpected response sequence {response_seq}"), None
        return types.Status.ok(), response

    @staticmethod
    def get_module(commutator: "Commutator") -> Optional["Messanger"]:
        for module_type, name2module in commutator.modules.items():
            if module_type.startswith(ModuleType.MESSANGER.value):
                return cast(Messanger, name2module["Messanger"])
        return None
