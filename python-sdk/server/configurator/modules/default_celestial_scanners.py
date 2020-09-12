from typing import Dict

from server.configurator.modules.celestial_scanner import CelestialScannerBlueprint


celestial_scanners_blueprints: Dict[str, CelestialScannerBlueprint] = {
    "basic": CelestialScannerBlueprint(name="Amateur Scanner",
                                       max_scanning_radius_km=1000,
                                       processing_time_us=10000)
}
