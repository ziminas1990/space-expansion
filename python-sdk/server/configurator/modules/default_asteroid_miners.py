from .asteroid_miner import AsteroidMinerBlueprint


def make_asteroid_miner_blueprint(prototype: str, cargo_name: str) -> AsteroidMinerBlueprint:
    """Create an miner's blueprint of the specified 'prototype', connected to the
    cargo with the specified 'cargo_name'"""
    if prototype == "Toy":
        return AsteroidMinerBlueprint(name="Toy Miner",
                                      max_distance=1000,
                                      cycle_time_ms=10000,
                                      yield_per_cycle=100,
                                      container=cargo_name)