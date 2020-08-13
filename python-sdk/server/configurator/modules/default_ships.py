from enum import Enum
from typing import Dict

import server.configurator.world as world
import server.configurator.modules as modules

from server.configurator.modules import (
    ShipBlueprint, Ship,
    engine_blueprints, EngineType, EngineSize,
    celestial_scanners_blueprints
)


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
            'scanner': celestial_scanners_blueprints["basic"].id
        }
    )
}


def make_probe(name: str, position: world.Position,
               main_engine: modules.Engine = modules.Engine(),
               additional_engine: modules.Engine = modules.Engine()):
    """Create configuration of the 'PROBE' ship"""
    return Ship(name=name,
                ship_type=ShipType.PROBE.value,
                position=position,
                modules={'main_engine': main_engine,
                         'additional_engine': additional_engine})


def make_miner(name: str, position: world.Position,
               main_engine: modules.Engine = modules.Engine(),
               additional_engine: modules.Engine = modules.Engine()):
    """Create configuration of the 'PROBE' ship"""
    return Ship(name=name,
                ship_type=ShipType.MINER.value,
                position=position,
                modules={'main_engine': main_engine,
                         'additional_engine': additional_engine})