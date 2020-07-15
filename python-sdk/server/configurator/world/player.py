from typing import Optional, Dict

from server.configurator.modules.ship import Ship


class Player:
    def __init__(self):
        self.login: Optional[str] = None
        self.password: Optional[str] = None
        self.ships: Dict[str, Ship] = {}

    def set_credentials(self, login: str, password: str) -> 'Player':
        self.login = login
        self.password = password
        return self

    def add_ship(self, name: str, ship: Ship) -> 'Player':
        key = f"{ship.ship_type}/{name}"
        assert key not in self.ships
        self.ships.update({key: ship})
        return self

    def verify(self):
        assert self.login and len(self.login) > 4
        assert self.password and len(self.password) > 4
        for name, ship in self.ships.items():
            assert len(name) > 0
            ship.verify()

    def to_pod(self):
        self.verify()
        return {
            "password": self.password,
            "ships": {name: ship.to_pod() for name, ship in self.ships.items()}
        }