from typing import Optional, List, Tuple, TYPE_CHECKING

from expansion.interfaces.rpc import (
    BlueprintsLibraryI,
    BlueprintsLibraryStatus
)
from expansion import utils
from .base_module import BaseModule, ModuleType
from expansion.types import Blueprint

if TYPE_CHECKING:
    from expansion.modules.base_module import TunnelFactory
    from .commutator import Commutator


class BlueprintsLibrary(BaseModule):

    def __init__(self,
                 tunnel_factory: "TunnelFactory",
                 name: Optional[str] = None):
        super().__init__(tunnel_factory=tunnel_factory,
                         name=name or utils.generate_name(BlueprintsLibrary))
        self.opened_port: Optional[Tuple[int, int]] = None  # (port, accessKey)

    def get_opened_port(self) -> Optional[Tuple[int, int]]:
        """Return (port, accessKey) pair, if port is opened. Otherwise
        return None"""
        return self.opened_port

    @BaseModule.use_session(
        terminal_type=BlueprintsLibraryI,
        return_on_unreachable=(BlueprintsLibraryStatus.UNREACHABLE, []),
        return_on_cancel=(BlueprintsLibraryStatus.CANCELED, []))
    async def get_blueprints_list(
            self,
            start_with: str = "",
            timeout: float = 0.5,
            session: Optional[BlueprintsLibraryI] = None)\
            -> (BlueprintsLibraryStatus, List[str]):
        assert session is not None
        return await session.get_blueprints_list(start_with, timeout)

    @BaseModule.use_session(
        terminal_type=BlueprintsLibraryI,
        return_on_unreachable=(BlueprintsLibraryStatus.UNREACHABLE, []),
        return_on_cancel=(BlueprintsLibraryStatus.CANCELED, []))
    async def get_blueprint(
            self,
            blueprint: str = "",
            timeout: float = 0.5,
            session: Optional[BlueprintsLibraryI] = None) \
            -> (BlueprintsLibraryStatus, Blueprint):
        assert session is not None
        return await session.get_blueprint(blueprint, timeout)

    @staticmethod
    def find(commutator: "Commutator") -> Optional["BlueprintsLibrary"]:
        return BaseModule._get_by_name(
            commutator=commutator,
            type=ModuleType.BLUEPRINTS_LIBRARY,
            name="Central"
        )
