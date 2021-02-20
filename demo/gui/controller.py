from typing import List
from tactical_core import TacticalCore

from expansion import modules


class Controller:
    # This class is an intermediary between GUI and tactical core
    # GUI informs controller about user's activity and controller
    # sends appropriate signals to the tactical core

    def __init__(self, tactical_core: TacticalCore):
        self.core = tactical_core

        self.selected_ships: List[modules.Ship] = []
        # All ships, selected by user

    def select_ships(self, selected_ships: List[modules.Ship]):
        self.selected_ships = selected_ships

    def left_click(self, x: float, y: float):
        if len(self.selected_ships) == 0:
            return
        # This action may be considered as an order for the selected
        # ships to move to the specified position
        for ship in self.selected_ships:
            self.core.ships_assistant.move_ship(ship=ship, x=x, y=y)
