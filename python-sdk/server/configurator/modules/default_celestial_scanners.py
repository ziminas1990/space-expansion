from typing import Dict

from .celestial_scanner import CelestialScannerBlueprint
from server.configurator.resources import ResourcesList, ResourceType


celestial_scanners_blueprints: Dict[str, CelestialScannerBlueprint] = {
    "basic": CelestialScannerBlueprint(
        name="Amateur Scanner",
        max_scanning_radius_km=1000,
        processing_time_us=10000,
        expenses=ResourcesList({
            ResourceType.e_METALS: 100,
            ResourceType.e_SILICATES: 100,
            ResourceType.e_LABOR: 20
        })),

    "station": CelestialScannerBlueprint(
        name="Station Scanner",
        max_scanning_radius_km=10000,
        processing_time_us=5000,
        expenses=ResourcesList({
            ResourceType.e_METALS: 700,
            ResourceType.e_SILICATES: 200,
            ResourceType.e_LABOR: 80
        })
    )
}
