import asyncio
from typing import List, TYPE_CHECKING

import world
from expansion import modules
from ship.navigator import Navigator

if TYPE_CHECKING:
    from expansion.modules import ModuleType


class Ship:
    def __init__(self,
                 remote: modules.Ship,
                 the_world: world.World,
                 system_clock: modules.SystemClock):
        self.remote: modules.Ship = remote
        self.world: world.World = the_world
        self.system_clock = system_clock
        self.name = remote.name

        # Crew:
        self.navigator: Navigator = Navigator(remote.name + ".navigator", remote, system_clock)

        # Active tasks:
        self.scanning_task = None

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

    def start_passive_scanning(self):
        async def scanning_procedure():
            scaner = await modules.PassiveScanner.get_most_ranged(self.remote)
            if not scaner:
                return
            async for detected_object in scaner.scan():
                self.world.update_object(detected_object)

        self.scanning_task = \
            asyncio.get_running_loop().create_task(scanning_procedure())