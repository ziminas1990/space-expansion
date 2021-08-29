from typing import Dict

from .asteroid_miner import AsteroidMinerBlueprint
from server.configurator.resources import ResourcesList, ResourceType

asteroid_miner_blueprints: Dict[str, AsteroidMinerBlueprint] = {
    "basic": AsteroidMinerBlueprint(
        name="Toy Miner",
        max_distance=500,
        cycle_time_ms=10000,
        yield_per_cycle=250,
        expenses=ResourcesList({
            ResourceType.e_METALS: 2000,
            ResourceType.e_SILICATES: 100,
            ResourceType.e_LABOR: 50
        }))
}
