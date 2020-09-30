from typing import Dict

from .asteroid_miner import AsteroidMinerBlueprint


asteroid_miner_blueprints: Dict[str, AsteroidMinerBlueprint] = {
    "basic": AsteroidMinerBlueprint(name="Toy Miner",
                                    max_distance=500,
                                    cycle_time_ms=10000,
                                    yield_per_cycle=250)
}
