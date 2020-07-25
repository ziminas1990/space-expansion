from typing import Dict
from .blueprints_db import BlueprintsDB
from .base_blueprint import BaseBlueprint, BlueprintId

from server.configurator.modules import default_engines_blueprints, default_ships_blueprints


class DefaultBlueprints(BlueprintsDB):
    """Blueprints database, that contains all default blueprints"""

    def __init__(self):
        blueprints: Dict[BlueprintId, BaseBlueprint] = {}
        # Engines:
        for size2engine in default_engines_blueprints.values():
            for engine_blueprint in size2engine.values():
                blueprints.update({engine_blueprint.id: engine_blueprint})
        # Ships
        for ship_blueprint in default_ships_blueprints.values():
            blueprints.update({ship_blueprint.id: ship_blueprint})

        super(DefaultBlueprints, self).__init__(blueprints=blueprints)
