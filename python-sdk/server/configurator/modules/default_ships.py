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
from server.configurator.resources import ResourcesList, ResourceType


class ShipType(Enum):
    PROBE = 'Probe'
    MINER = 'Miner'
    STATION = 'Station'


ships_blueprints: Dict[ShipType, ShipBlueprint] = {
    ShipType.PROBE: ShipBlueprint(
        name="Probe",
        radius=2,
        weight=220,
        modules={
            'main_engine': engine_blueprints[EngineType.CHEMICAL][EngineSize.TINY].id,
            'additional_engine': engine_blueprints[EngineType.ION][EngineSize.TINY].id
        },
        expenses=ResourcesList({
            ResourceType.e_METALS: 200,
            ResourceType.e_SILICATES: 20,
            ResourceType.e_LABOR: 100
        })
    ),

    ShipType.MINER: ShipBlueprint(
        name="Miner",
        radius=80,
        weight=80000,
        modules={
            'main_engine': engine_blueprints[EngineType.NUCLEAR][EngineSize.REGULAR].id,
            'additional_engine': engine_blueprints[EngineType.ION][EngineSize.REGULAR].id,
            'scanner': celestial_scanners_blueprints["basic"].id,
            'cargo': resource_containers_blueprints["small"].id,
            'tiny_cargo': resource_containers_blueprints["tiny"].id,
            'miner': asteroid_miner_blueprints["basic"].id
        },
        expenses=ResourcesList({
            ResourceType.e_METALS: 70000,
            ResourceType.e_SILICATES: 10000,
            ResourceType.e_LABOR: 20000
        })
    ),

    ShipType.STATION: ShipBlueprint(
            name="Station",
            radius=800,
            weight=20000000,
            modules={
                'engine': engine_blueprints[EngineType.NUCLEAR][EngineSize.REGULAR].id,
                'scanner': celestial_scanners_blueprints["station"].id,
                'warehouse': resource_containers_blueprints["station"].id,
                'shipyard-container': resource_containers_blueprints["station"].id,
            },
            expenses=ResourcesList({
                ResourceType.e_METALS: 17000000,
                ResourceType.e_SILICATES: 3000000,
                ResourceType.e_LABOR: 3000000
            })
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
               cargo: ResourceContainerState = ResourceContainerState(),
               tiny_cargo: ResourceContainerState = ResourceContainerState()):
    """Create configuration of the 'MINER' ship"""
    return Ship(name=name,
                ship_type=ShipType.MINER.value,
                position=position,
                modules={"main_engine": main_engine,
                         "additional_engine": additional_engine,
                         "cargo": cargo,
                         "tiny_cargo": tiny_cargo})


def make_station(name: str,
                 position: world.Position,
                 engine: EngineState = EngineState(),
                 shipyard_container: ResourceContainerState = ResourceContainerState()):
    """Create configuration of the 'STATION' ship"""
    return Ship(name=name,
                ship_type=ShipType.STATION.value,
                position=position,
                modules={
                    "engine": engine,
                    "shipyard-container": shipyard_container
                })
