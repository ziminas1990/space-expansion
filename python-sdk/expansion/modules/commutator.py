from typing import Dict, List, Optional, Callable, Awaitable, Tuple

import expansion.interfaces.public as rpc
from expansion.interfaces.public.commutator import Tunnel
from expansion import utils

from .base_module import BaseModule, ModuleType

TunnelOrError = Tuple[Optional[Tunnel], Optional[str]]
ModuleOrError = Tuple[Optional[BaseModule], Optional[str]]
ConnectionFactory = Callable[[], Awaitable[rpc.CommutatorI]]
TunnelFactory = Callable[[], Awaitable[TunnelOrError]]
ModulesFactory = Callable[[str, str, TunnelFactory], ModuleOrError]


class Commutator(BaseModule):
    def __init__(self,
                 connection_factory: ConnectionFactory,
                 modules_factory: ModulesFactory,
                 name: Optional[str] = None):
        self.name = name or utils.generate_name(Commutator)
        super().__init__(connection_factory=connection_factory,
                         logging_name=self.name)
        self.modules_factory = modules_factory
        self.modules_info: Dict[str, Dict[str, int]] = {}
        # Map: module_type -> module_name -> slot_id
        self.modules: Dict[str, Dict[str, BaseModule]] = {}
        # Map: module_type -> module_name -> module

    async def init(self) -> bool:
        """Retrieve an information about all modules, attached to the
        commutator. Should be called after module is instantiated"""
        self.modules_info.clear()

        async with self._lock_channel() as channel:
            assert isinstance(channel, rpc.CommutatorI)
            modules: List[rpc.ModuleInfo] = await channel.get_all_modules()

        if modules is None:
            self.logger.warning(f"Failed to get modules list!")
            return False

        for module in modules:
            modules_of_type = self.modules_info.setdefault(module.type, {})
            modules_of_type.update({
                module.name: module.slot_id
            })
            self._add_module(module.type, module.name, module.slot_id)

        for module_type, name2module in self.modules.items():
            for module_name, module in name2module.items():
                if not await module.init():
                    self.logger.warning(f"Can't init {module_type} '{module_name}'")

        return True

    def _add_module(self, module_type: str, name: str, slot_id: int):
        async def tunnel_factory():
            async with self._lock_channel() as channel:
                assert isinstance(channel, rpc.CommutatorI)
                return await channel.open_tunnel(port=slot_id)

        module_instance, error = self.modules_factory(module_type, name, tunnel_factory)
        if error is not None:
            self.logger.warning(f"Failed to connect to {module_type} '{name}': "
                                f"{error}!")
            return

        try:
            modules = self.modules[module_type]
            modules.update({
                name: module_instance
            })
        except KeyError:
            self.modules.update({
                module_type: {name: module_instance}
            })
