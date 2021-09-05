from typing import Dict

from server.configurator.modules import ShipyardBlueprint
from server.configurator.resources import ResourcesList, ResourceType


shipyard_blueprints: Dict[str, ShipyardBlueprint] = {
    "small": ShipyardBlueprint(
        name="Small Shipyard",
        productivity=5,
        container_name="shipyard-container",
        expenses=ResourcesList({
            ResourceType.e_METALS: 20000,
            ResourceType.e_SILICATES: 10000,
            ResourceType.e_LABOR: 25000
        })
    ),
    "medium": ShipyardBlueprint(
        name="Medium Shipyard",
        productivity=10,
        container_name="shipyard-container",
        expenses=ResourcesList({
            ResourceType.e_METALS: 33000,
            ResourceType.e_SILICATES: 15000,
            ResourceType.e_LABOR: 45000
        })),
    "large": ShipyardBlueprint(
        name="Large Shipyard",
        productivity=25,
        container_name="shipyard-container",
        expenses=ResourcesList({
            ResourceType.e_METALS: 60000,
            ResourceType.e_SILICATES: 30000,
            ResourceType.e_LABOR: 80000
        }))
}
