from typing import Optional, NamedTuple, List

import expansion.protocol as protocol
import expansion.transport as transport
import expansion.utils as utils
import expansion.types as types


class Specification(NamedTuple):
    max_radius_km: int
    processing_time_us: int


class CelestialScannerI(transport.IOTerminal):

    def __init__(self, name: Optional[str] = None):
        super().__init__(name=name or utils.generate_name(CelestialScannerI))
        self.specification: Optional[Specification] = None

    async def expected_scanning_time(self, scanning_radius_km: int, minimal_radius_m: int) -> float:
        """Return time, that should be required by scanner to finish scanning
        request with the specified 'scanning_radius_km' and 'minimal_radius_m'"""
        spec = await self.get_specification()
        if not spec:
            return 0
        resolution = (1000 * scanning_radius_km) / minimal_radius_m
        c_km_per_sec = 300000
        total_processing_time = (resolution * spec.processing_time_us) / 1000000
        return 0.1 + 2 * scanning_radius_km / c_km_per_sec + total_processing_time

    async def get_specification(self, timeout: float = 0.5)\
            -> Optional[Specification]:
        request = protocol.Message()
        request.celestial_scanner.specification_req = True
        if not self.send(message=request):
            return None
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return None
        spec = protocol.get_message_field(response, "celestial_scanner.specification")
        if not spec:
            return None
        return Specification(max_radius_km=spec.max_radius_km,
                             processing_time_us=spec.processing_time_us)

    async def scan(self,
                   scanning_radius_km: int,
                   minimal_radius_m: int,
                   timeout: float) -> (Optional[List[types.PhysicalObject]], Optional[str]):
        """Scanning all bodies with a signature not less than 'minimal_radius_m'
        within a 'scanning_radius_km' radius.
        On success return (asteroid_list, None). Otherwise return (None, error_string)
        """
        request = protocol.Message()
        scan_req = request.celestial_scanner.scan
        scan_req.scanning_radius_km = scanning_radius_km
        scan_req.minimal_radius_m = minimal_radius_m
        if not self.send(message=request):
            return None, "Failed to send message"

        scanned_objects: List[types.PhysicalObject] = []

        def handle_response(response) -> (int, Optional[str]):
            """Handle response. Return (asteroids_left, None) on success,
            otherwise return (0, error)"""
            timestamp: Optional[int] = response.timestamp or None

            report = protocol.get_message_field(response, "celestial_scanner.scanning_report")
            if report:
                for body in report.asteroids:
                    scanned_objects.append(
                        types.PhysicalObject(
                            object_id=body.id,
                            position=types.Position(
                                x=body.x, y=body.y,
                                velocity=types.Vector(x=body.vx, y=body.vy),
                                timestamp=timestamp
                            ),
                        )
                    )
                return report.left, None

            fail = protocol.get_message_field(response, "celestial_scanner.scanning_failed")
            if fail:
                return 0, str(fail)
            return 0, f"Got unexpected response '{response.WhichOneof('choice')}'"

        while True:
            response, _ = await self.wait_message(timeout=timeout)
            if not response:
                return 0, "No response"
            objects_left, error = handle_response(response)
            if error:
                return 0, error
            if objects_left == 0:
                return scanned_objects, None
