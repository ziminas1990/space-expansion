import logging

from typing import Dict, List, TYPE_CHECKING

if TYPE_CHECKING:
    from ship import Ship
    from expansion.modules import ModuleType


class Player:
    def __init__(self):
        self._logger = logging.getLogger("World")
        self.ships: Dict[str, "Ship"] = {}

    def get_ships_by_equipment(
            self,
            modules: List["ModuleType"]):
        ships: List["Ship"] = []
        for ship in self.ships.values():
            if ship.has_modules(modules):
                ships.append(ship)
        return ships

