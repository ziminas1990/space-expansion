from enum import Enum
from typing import Dict, Any

from .engine import EngineBlueprint


class EngineType(Enum):
    CHEMICAL = 'Chemical'
    ION = 'Ion'
    NUCLEAR = 'Nuclear'


class EngineSize(Enum):
    TINY = 'Tiny'
    SMALL = 'Small'


engine_parameters: Dict[EngineType, Dict[EngineSize, Dict[str, Any]]] = {
    EngineType.CHEMICAL: {
        EngineSize.TINY: {'max_thrust': 1000},
        EngineSize.SMALL: {'max_thrust': 10000}
    },
    EngineType.ION: {
        EngineSize.TINY: {'max_thrust': 100},
        EngineSize.SMALL: {'max_thrust': 500}
    },
    EngineType.NUCLEAR: {
        EngineSize.TINY: {'max_thrust': 3000},
        EngineSize.SMALL: {'max_thrust': 30000}
    },
}


def _create_blueprints() -> Dict[EngineType, Dict[EngineSize, EngineBlueprint]]:
    return {
        engine_type: {
            engine_size: EngineBlueprint(
                name=f"{engine_size.value} {engine_type.value} engine",
                **params)
            for engine_size, params in engine_parameters[engine_type].items()
        } for engine_type in engine_parameters.keys()
    }


engine_blueprints: Dict[EngineType, Dict[EngineSize, EngineBlueprint]] = _create_blueprints()
