from enum import Enum
from typing import Dict

import server.configurator.world as world

from .engine import EngineState
from .resource_container import ResourceContainerState
from .default_resource_containers import resource_containers_blueprints
from .ship import ShipBlueprint, Ship
from .default_engines import engine_blueprints, EngineType, EngineSize
from .default_celestial_scanners import celestial_scanners_blueprints
from .default_asteroid_miners import asteroid_miner_blueprints


class ShipType(Enum):
    PROBE = 'Probe'
    MINER = 'Miner'


ships_blueprints: Dict[ShipType, ShipBlueprint] = {
    ShipType.PROBE: ShipBlueprint(
        name="Probe",
        radius=2,
        weight=220,
        modules={
            'main_engine': engine_blueprints[EngineType.CHEMICAL][EngineSize.TINY].id,
            'additional_engine': engine_blueprints[EngineType.ION][EngineSize.TINY].id
        }
    ),

    ShipType.MINER: ShipBlueprint(
        name="Miner",
        radius=80,
        weight=112000,
        modules={
            'main_engine': engine_blueprints[EngineType.NUCLEAR][EngineSize.REGULAR].id,
            'additional_engine': engine_blueprints[EngineType.ION][EngineSize.REGULAR].id,
            'scanner': celestial_scanners_blueprints["basic"].id,
            'cargo': resource_containers_blueprints["small"].id,
            'miner': asteroid_miner_blueprints["basic"].id
        }
    )
}


def make_probe(name: str, position: world.Position,
               main_engine: EngineState = EngineState(),
               additional_engine: EngineState = EngineState()):
    """Create configuration of the 'PROBE' ship"""
    return Ship(name=name,
                ship_type=ShipType.PROBE.value,
                position=position,
                modules={'main_engine': main_engine,
                         'additional_engine': additional_engine})


def make_miner(name: str, position: world.Position,
               main_engine: EngineState = EngineState(),
               additional_engine: EngineState = EngineState(),
               cargo: ResourceContainerState = ResourceContainerState()):
    """Create configuration of the 'MINER' ship"""
    return Ship(name=name,
                ship_type=ShipType.MINER.value,
                position=position,
                modules={"main_engine": main_engine,
                         "additional_engine": additional_engine,
                         "cargo": cargo})
