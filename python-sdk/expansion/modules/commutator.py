import asyncio
from typing import (
    Dict,
    List,
    Set,
    Optional,
    Callable,
    Tuple,
    Iterable,
    AsyncIterable,
    TYPE_CHECKING
)

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
        # Map: slot_id -> (module_type, module_name)
        self.slots: Dict[int, Tuple[str, str]] = {}
        # Map: module_type -> module_name -> module
        self.modules: Dict[str, Dict[str, BaseModule]] = {}

        self._monitoring_task = None

    async def update(self) -> bool:
        """Request modules list from server and check if any modules were
        added or removed. Do the synchronization."""

        modules: Optional[List[rpc.ModuleInfo]] = await self._get_all_modules()
        if modules is None:
            self.logger.warning(f"Failed to get modules list!")
            return False

        for module in modules:
            try:
                existing = self.slots[module.slot_id]
                if existing[0] != module.type or existing[1] != module.name:
                    # Old module was replaced with a new one
                    self._on_module_detached(module.slot_id)
                    await self._on_module_attached(module)
            except KeyError:
                # new module
                await self._on_module_attached(module)
        # TODO: Check if any slots were detached
        return True

    async def init(self) -> bool:
        """Retrieve an information about all modules, attached to the
        commutator. Should be called after module is instantiated"""
        return await self.update()

    @BaseModule.use_session_for_generators(
        terminal_type=rpc.CommutatorI,
        return_on_unreachable=None
    )
    async def monitoring(self, session: Optional[rpc.CommutatorI] = None) \
            -> AsyncIterable[Optional[rpc.CommutatorUpdate]]:
        status: rpc.CommutatorI.Status = await session.monitor()
        while status.is_success():
            # Return None immediately once monitoring has started.
            # It doesn't break a contract but let client know that
            # monitoring procedure has started
            yield None
            status, update = await session.wait_update(timeout=0.5)
            if update:
                # Update self first, then return an update
                if update.module_attached:
                    await self._on_module_attached(update.module_attached)
                elif update.module_detached:
                    self._on_module_detached(update.module_detached)
                yield update
            else:
                yield None

    async def _on_module_attached(self, module: rpc.ModuleInfo):
        module_instance = self.instantiate_module(module)
        if module_instance:
            if await module_instance.init():
                self.modules.setdefault(module.type, {}).update({
                    module.name: module_instance
                })
                self.slots.update({
                    module.slot_id: (module.type, module.name)
                })
            else:
                self.logger.warning(f"Can't init {module.type} '{module.name}'")

    def _on_module_detached(self, slot_id: int):
        try:
            module_type, module_name = self.slots[slot_id]
            del self.modules[module_type][module_name]
        except KeyError:
            return

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
        # What will happen:
        # 1. send an 'open_tunnel' request to the server
        # 2. server sends 'open_tunnel_report'
        # 3. when SessionMux see the 'open_tunnel_report' it spawns a new
        #    corresponding session for it and pass the report to uplevel as
        #    usual
        # 4. here, we get and return the session, created in step 3
        status, session_id = await session.open_tunnel(port=slot_id)
        if status.is_success():
            return self.session_mux.get_channel_for_session(session_id), None
        else:
            return None, str(status)

    def instantiate_module(self, module_info: rpc.ModuleInfo) \
            -> Optional[BaseModule]:
        async def tunnel_factory():
            return await self._open_tunnel(module_info.slot_id)

        module_instance, error = self.modules_factory(
            module_info.type,
            module_info.name,
            self.session_mux,
            tunnel_factory)
        if error is None:
            return module_instance
        else:
            self.logger.warning(f"Failed to connect to {module_info.type} "
                                f"'{module_info.name}': {error}!")
            return None
