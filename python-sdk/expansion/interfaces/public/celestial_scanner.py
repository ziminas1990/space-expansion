from typing import Optional, NamedTuple, List, Callable, Any

import expansion.protocol as protocol
import expansion.transport as transport
import expansion.utils as utils
import expansion.types as types


class Specification(NamedTuple):
    max_radius_km: int
    processing_time_us: int


ObjectsList = Optional[List[types.PhysicalObject]]
Error = Optional[str]


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
                   result_cb: Callable[[ObjectsList, Error], None],
                   timeout: float) -> Optional[str]:
        """Scanning all bodies within a 'scanning_radius_km' radius.

        Bodies with radius less then the specified 'minimal_radius_m' will be
        ignored. The 'result_cb' callback will be called when another portion
        of data received from the server. After all data received callback
        will be called for the last time with (None, None) argument.
        Retun None on success otherwise return error string
        """
        request = protocol.Message()
        scan_req = request.celestial_scanner.scan
        scan_req.scanning_radius_km = scanning_radius_km
        scan_req.minimal_radius_m = minimal_radius_m
        if not self.send(message=request):
            error = "Failed to send request"
            result_cb(None, error)
            return error

        continue_scanning = True
        while continue_scanning:
            response, _ = await self.wait_message(timeout=timeout)
            body = protocol.get_message_field(response, "celestial_scanner")
            if not body:
                error = "No response"
                result_cb(None, error)
                return error

            report = protocol.get_message_field(body, "scanning_report")
            if not report:
                fail = protocol.get_message_field(body, "scanning_failed")
                error = str(fail) if fail else self.__unexpected_msg_str(fail)
                result_cb(None, error)
                return error

            timestamp: Optional[int] = response.timestamp or None
            result_cb(
                [self.__build_object(body, timestamp) for body in report.asteroids],
                None
            )
            continue_scanning = report.left > 0

        result_cb(None, None)  # Scanning is finished
        return None

    async def scan_sync(self,
                        scanning_radius_km: int,
                        minimal_radius_m: int,
                        timeout: float) -> (ObjectsList, Error):
        """Scanning all bodies within a 'scanning_radius_km' radius.

        Bodies with radius less then the specified 'minimal_radius_m' will be
        ignored. On success return (asteroid_list, None). Otherwise
        return (None, error_string)
        """
        scanned_objects: AsteroidsList = []
        error_msg: Error = None

        def accumulator(asteroids: ObjectsList, error: Error):
            nonlocal error_msg
            if asteroids:
                scanned_objects.extend(asteroids)
            elif error:
                error_msg = error

        await self.scan(scanning_radius_km,
                        minimal_radius_m,
                        accumulator,
                        timeout)
        return scanned_objects, error_msg

    @staticmethod
    def __build_object(data: Any,  # Because its protobuf's type
                       timestamp: Optional[int]):
        return types.PhysicalObject(
            object_id=data.id,
            position=types.Position(
                x=data.x, y=data.y,
                velocity=types.Vector(x=data.vx, y=data.vy),
                timestamp=timestamp
            ),
            radius=data.r
        )

    @staticmethod
    def __unexpected_msg_str(message: Any):
        return f"Got unexpected response '{message.WhichOneof('choice')}'"
