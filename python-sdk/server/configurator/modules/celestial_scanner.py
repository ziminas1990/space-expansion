from typing import Optional
from server.configurator.blueprints.base_blueprint import BaseBlueprint, BlueprintId, ModuleType
from server.configurator.modules import BaseModule
from server.configurator.world.geomtery import Vector


class CelestialScannerBlueprint(BaseBlueprint):
    def __init__(self, name: str,
                 max_scanning_radius_km: Optional[int] = None,
                 processing_time_us: Optional[int] = None):
        super().__init__(blueprint_id=BlueprintId(ModuleType.e_CELESTIAL_SCANNER, name))
        self.max_scanning_radius_km: Optional[int] = max_scanning_radius_km
        self.processing_time_us: Optional[int] = processing_time_us

    def verify(self):
        super().verify()
        assert self.max_scanning_radius_km is not None and self.max_scanning_radius_km > 0
        assert self.processing_time_us is not None and self.processing_time_us > 0

    def to_pod(self):
        self.verify()
        data = super(CelestialScannerBlueprint, self).to_pod()
        data.update({
            'max_scanning_radius_km': self.max_scanning_radius_km,
            'processing_time_us': self.processing_time_us,
        })
        return data
