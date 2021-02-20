from typing import Optional, Dict
import logging

import expansion.modules as modules
from .ship_computer import ShipComputer


class ShipsAssistant:
    def __init__(self,
                 root_commutator: modules.Commutator,
                 system_clock: modules.SystemClock):
        self.logger = logging.getLogger("ShipsAssistant")
        self.commutator: modules.commutator = root_commutator
        self.system_clock: modules.SystemClock = system_clock
        self.ships: Dict[str, modules.Ship] = {}
        self.ships_computers: Dict[modules.Ship, ShipComputer] = {}

    def get_ship_by_name(self, name: str) -> Optional[modules.Ship]:
        try:
            return self.ships[name]
        except KeyError:
            return None

    def move_ship(self, ship: modules.Ship, x: float, y: float):
        computer = self.ships_computers.setdefault(
            ship, ShipComputer(ship=ship, system_clock=self.system_clock))
        computer.move_to(x, y)

    async def sync(self):
        ships = modules.get_all_ships(self.commutator)
        self.ships = {
            ship.name: ship for ship in ships
        }

        for ship in self.ships.values():
            if not await ship.sync():
                self.logger.warning(f"Failed to sync ship {ship.name}")
