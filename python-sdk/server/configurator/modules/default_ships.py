from enum import Enum
from typing import Dict

import server.configurator.world as world
import server.configurator.modules as modules

from .ship import ShipBlueprint, Ship
from .default_engines import engine_blueprints, EngineType, EngineSize


class ShipType(Enum):
    PROBE = 'Probe'


ships_blueprints: Dict[ShipType, ShipBlueprint] = {
    ShipType.PROBE: ShipBlueprint(
        name="Probe",
        radius=2,
        weight=220,
        modules={
            'main_engine': engine_blueprints[EngineType.CHEMICAL][EngineSize.TINY].id,
            'additional_engine': engine_blueprints[EngineType.ION][EngineSize.TINY].id
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