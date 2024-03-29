from typing import Dict
from .blueprints_db import BlueprintsDB
from .base_blueprint import BaseBlueprint, BlueprintId

from server.configurator.modules import (
    engine_blueprints,
    ships_blueprints,
    celestial_scanners_blueprints,
    passive_scanners_blueprints,
    resource_containers_blueprints,
    asteroid_miner_blueprints,
    shipyard_blueprints
)


class DefaultBlueprints(BlueprintsDB):
    """Blueprints database, that contains all default blueprints"""

    def __init__(self):
        blueprints: Dict[BlueprintId, BaseBlueprint] = {}
        # Engines:
        for size2engine in engine_blueprints.values():
            for engine_blueprint in size2engine.values():
                blueprints.update({engine_blueprint.id: engine_blueprint})
        # CelestialScanner
        for scanner_blueprint in celestial_scanners_blueprints.values():
            blueprints.update({scanner_blueprint.id: scanner_blueprint})
        # PassiveScanners
        for scanner_blueprint in passive_scanners_blueprints.values():
            blueprints.update({scanner_blueprint.id: scanner_blueprint})
        # ResourceContainer
        for blueprint in resource_containers_blueprints.values():
            blueprints.update({blueprint.id: blueprint})
        # AsteroidMiners
        for blueprint in asteroid_miner_blueprints.values():
            blueprints.update({blueprint.id: blueprint})
        # Shipyard
        for blueprint in shipyard_blueprints.values():
            blueprints.update({blueprint.id: blueprint})
        # Ships
        for ship_blueprint in ships_blueprints.values():
            blueprints.update({ship_blueprint.id: ship_blueprint})
        super(DefaultBlueprints, self).__init__(blueprints=blueprints)
