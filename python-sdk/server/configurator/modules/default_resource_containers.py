from typing import Dict

from .resource_container import ResourceContainerBlueprint
from server.configurator.resources import ResourcesList, ResourceType


resource_containers_blueprints: Dict[str, ResourceContainerBlueprint] = {
    "tiny": ResourceContainerBlueprint(
        name="Tiny Resource Container",
        volume=20,
        expenses=ResourcesList({
            ResourceType.e_METALS: 400,
            ResourceType.e_SILICATES: 6,
            ResourceType.e_LABOR: 20
        })
    ),
    "small": ResourceContainerBlueprint(
        name="Small Resource Container",
        volume=125,
        expenses=ResourcesList({
            ResourceType.e_METALS: 1200,
            ResourceType.e_SILICATES: 25,
            ResourceType.e_LABOR: 80
        })
    ),
    "medium": ResourceContainerBlueprint(
        name="Medium Resource Container",
        volume=500,
        expenses=ResourcesList({
            ResourceType.e_METALS: 3000,
            ResourceType.e_SILICATES: 60,
            ResourceType.e_LABOR: 250
        })
    ),
    "station": ResourceContainerBlueprint(
        name="Station Resource Container",
        volume=1500,
        expenses=ResourcesList({
            ResourceType.e_METALS: 5000,
            ResourceType.e_SILICATES: 140,
            ResourceType.e_LABOR: 600
        })
    )
}
