from typing import Dict

from server.configurator.modules.passive_scanner import PassiveScannerBlueprint
from server.configurator.resources import ResourcesList, ResourceType


passive_scanners_blueprints: Dict[str, PassiveScannerBlueprint] = {
    "basic": PassiveScannerBlueprint(
        name="Basic Scanner",
        max_scanning_radius_km=50,
        edge_update_time_ms=2000,
        expenses=ResourcesList({
            ResourceType.e_METALS: 200,
            ResourceType.e_SILICATES: 200,
            ResourceType.e_LABOR: 20
        })),

    "military": PassiveScannerBlueprint(
        name="Military Scanner",
        max_scanning_radius_km=150,
        edge_update_time_ms=40000,
        expenses=ResourcesList({
            ResourceType.e_METALS: 800,
            ResourceType.e_SILICATES: 800,
            ResourceType.e_LABOR: 80
        })),

    "station": PassiveScannerBlueprint(
        name="Station Scanner",
        max_scanning_radius_km=250,
        edge_update_time_ms=6000,
        expenses=ResourcesList({
            ResourceType.e_METALS: 2500,
            ResourceType.e_SILICATES: 2500,
            ResourceType.e_LABOR: 400
        })
    )
}
