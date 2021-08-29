from typing import Optional, Dict, TYPE_CHECKING

from server.configurator.blueprints.base_blueprint import BlueprintId, ModuleType

if TYPE_CHECKING:
    from server.configurator import General
    from server.configurator.blueprints import BlueprintsDB
    from server.configurator.world.player import Player
    from server.configurator.world.world import World


class Configuration:
    def __init__(self,
                 general: Optional["General"] = None,
                 blueprints: Optional["BlueprintsDB"] = None,
                 world: Optional["World"] = None,
                 players: Dict[str, "Player"] = {}):
        self.general: Optional["General"] = general
        self.blueprints: Optional["BlueprintsDB"] = blueprints
        self.world: Optional["World"] = world
        self.players: Dict[str, "Player"] = players

    def set_general(self, general: "General") -> 'Configuration':
        self.general = general
        return self

    def set_blueprints(self, blueprints: "BlueprintsDB") -> 'Configuration':
        self.blueprints = blueprints
        return self

    def add_player(self, player: "Player") -> 'Configuration':
        assert player.login not in self.players
        self.players.update({player.login: player})
        return self

    def set_world(self, world: "World") -> 'Configuration':
        self.world = world
        return self

    def verify(self):
        assert self.general
        assert self.blueprints
        assert self.world
        self.general.verify()
        self.blueprints.verify()
        self.world.verify()
        for login, player in self.players.items():
            assert len(login) > 3
            player.verify()
        self._deep_verification()

    def to_pod(self):
        self.verify()
        return {
            "application": self.general.to_pod(),
            "Blueprints": self.blueprints.to_pod(),
            "Players": {login: player.to_pod() for login, player in self.players.items()},
            "World": self.world.to_pod()
        }

    def _deep_verification(self):
        # Check, that all configured modules on a ship has an appropriate
        # blueprint. Note that not every module on the ship may be configured
        for player in self.players.values():
            for ship in player.ships.values():
                blueprint_id = BlueprintId(ModuleType.e_SHIP, ship.ship_type)
                assert blueprint_id in self.blueprints.blueprints,\
                    f"The '{blueprint_id}' blueprint doesn't exist!"
