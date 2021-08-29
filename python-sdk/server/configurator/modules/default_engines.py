from enum import Enum
from typing import Dict, Any

from .engine import EngineBlueprint
from server.configurator.resources import ResourcesList, ResourceType


class EngineType(Enum):
    CHEMICAL = "Chemical"
    ION = "Ion"
    NUCLEAR = "Nuclear"


class EngineSize(Enum):
    TINY = "Tiny"
    SMALL = "Small"
    REGULAR = "Regular"


engine_parameters: Dict[EngineType, Dict[EngineSize, Dict[str, Any]]] = {
    EngineType.CHEMICAL: {
        EngineSize.TINY: {
            "max_thrust": 1000,
            "expenses": ResourcesList({
                ResourceType.e_METALS: 200,
                ResourceType.e_SILICATES: 100,
                ResourceType.e_LABOR: 20
            })
        },
        EngineSize.SMALL: {
            "max_thrust": 10000,
            "expenses": ResourcesList({
                ResourceType.e_METALS: 600,
                ResourceType.e_SILICATES: 200,
                ResourceType.e_LABOR: 50
            })
        }
    },
    EngineType.ION: {
        EngineSize.TINY: {
            "max_thrust": 100,
            "expenses": ResourcesList({
                ResourceType.e_METALS: 100,
                ResourceType.e_SILICATES: 20,
                ResourceType.e_LABOR: 20
            })
        },
        EngineSize.SMALL: {
            "max_thrust": 500,
            "expenses": ResourcesList({
                ResourceType.e_METALS: 300,
                ResourceType.e_SILICATES: 50,
                ResourceType.e_LABOR: 60
            })
        },
        EngineSize.REGULAR: {
            "max_thrust": 2000,
            "expenses": ResourcesList({
                ResourceType.e_METALS: 1000,
                ResourceType.e_SILICATES: 120,
                ResourceType.e_LABOR: 180
            })
        },
    },
    EngineType.NUCLEAR: {
        EngineSize.TINY: {
            "max_thrust": 3000,
            "expenses": ResourcesList({
                ResourceType.e_METALS: 1500,
                ResourceType.e_SILICATES: 400,
                ResourceType.e_LABOR: 200
            })
        },
        EngineSize.SMALL: {
            "max_thrust": 30000,
            "expenses": ResourcesList({
                ResourceType.e_METALS: 10000,
                ResourceType.e_SILICATES: 2800,
                ResourceType.e_LABOR: 1600
            })
        },
        EngineSize.REGULAR: {
            "max_thrust": 400000,
            "expenses": ResourcesList({
                ResourceType.e_METALS: 35000,
                ResourceType.e_SILICATES: 15000,
                ResourceType.e_LABOR: 9500
            })
        }
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
