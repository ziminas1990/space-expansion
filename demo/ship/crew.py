from expansion import modules
from ship import Navigator


class Crew:

    def __init__(self, ship: modules.Ship, system_clock: modules.SystemClock):
        self.navigator: Navigator = Navigator(
            name=f"{ship.name}.Navigator",
            ship=ship,
            system_clock=system_clock
        )
