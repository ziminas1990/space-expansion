from typing import Dict, List, Optional, Callable, Tuple, TYPE_CHECKING

import expansion.interfaces.rpc as rpc
from expansion.transport import SessionsMux
from expansion import utils

from .base_module import BaseModule, TunnelFactory

if TYPE_CHECKING:
    from expansion.transport import Channel

ModuleOrError = Tuple[Optional[BaseModule], Optional[str]]
ModulesFactory = Callable[[str, str, SessionsMux, TunnelFactory], ModuleOrError]


class Commutator(BaseModule):
    def __init__(self,
                 session_mux: SessionsMux,
                 tunnel_factory: TunnelFactory,
                 modules_factory: ModulesFactory,
                 name: Optional[str] = None):
        self.name = name or utils.generate_name(Commutator)
        super().__init__(tunnel_factory=tunnel_factory,
                         name=self.name)
        self.session_mux = session_mux
        self.modules_factory = modules_factory
        self.modules_info: Dict[str, Dict[str, int]] = {}
        # Map: module_type -> module_name -> slot_id
        self.modules: Dict[str, Dict[str, BaseModule]] = {}
        # Map: module_type -> module_name -> module

    async def init(self) -> bool:
        """Retrieve an information about all modules, attached to the
        commutator. Should be called after module is instantiated"""
        self.modules_info.clear()

        modules: Optional[List[rpc.ModuleInfo]] = await self._get_all_modules()

        if modules is None:
            self.logger.warning(f"Failed to get modules list!")
            return False

        for module in modules:
            modules_of_type = self.modules_info.setdefault(module.type, {})
            modules_of_type.update({
                module.name: module.slot_id
            })
            self.add_module(module.type, module.name, module.slot_id)

        for module_type, name2module in self.modules.items():
            for module_name, module in name2module.items():
                if not await module.init():
                    self.logger.warning(f"Can't init {module_type} '{module_name}'")

        return True

    @BaseModule.use_session(
        terminal_type=rpc.CommutatorI,
        return_on_unreachable=None,
        return_on_cancel=None)
    async def _get_all_modules(self, session: Optional[rpc.CommutatorI] = None) \
            -> Optional[List[rpc.ModuleInfo]]:
        return await session.get_all_modules()

    @BaseModule.use_session(
        terminal_type=rpc.CommutatorI,
        return_on_unreachable=(None, "Unreachable"),
        return_on_cancel=(None, "Canceled"))
    async def _open_tunnel(self,
                           slot_id: int,
                           session: Optional[rpc.CommutatorI] = None) \
            -> Tuple[Optional["Channel"], Optional[str]]:
        status, session_id = await session.open_tunnel(port=slot_id)
        if status.is_success():
            return self.session_mux.get_channel_for_session(session_id), None
        else:
            return None, str(status)

    def add_module(self, module_type: str, name: str, slot_id: int) -> bool:
        async def tunnel_factory():
            return await self._open_tunnel(slot_id)

        module_instance, error = self.modules_factory(
            module_type,
            name,
            self.session_mux,
            tunnel_factory)

        if error is not None:
            self.logger.warning(f"Failed to connect to {module_type} '{name}': "
                                f"{error}!")
            return False

        try:
            modules = self.modules[module_type]
            modules.update({
                name: module_instance
            })
        except KeyError:
            self.modules.update({
                module_type: {name: module_instance}
            })
        return True
