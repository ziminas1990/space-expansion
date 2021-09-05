from typing import List, TYPE_CHECKING
from expansion import modules
from ship.navigator import Navigator

if TYPE_CHECKING:
    from expansion.modules import ModuleType


class Ship:
    def __init__(self,
                 remote: modules.Ship,
                 system_clock: modules.SystemClock):
        self.remote: modules.Ship = remote
        self.system_clock = system_clock
        self.name = remote.name

        # Crew:
        self.navigator: Navigator = Navigator(remote.name + ".navigator", remote, system_clock)

        # Forwarding some functions:
        self.get_position = self.remote.get_position
        self.predict_position = self.remote.predict_position

    def commutator(self) -> modules.Commutator:
        return self.remote

    def has_module(self, module_type: "ModuleType") -> bool:
        return module_type.value in self.remote.modules and \
               self.remote.modules[module_type.value]

    def has_modules(self, module_types: List["ModuleType"]) -> bool:
        for module_type in module_types:
            if not self.has_module(module_type):
                return False
        return True
