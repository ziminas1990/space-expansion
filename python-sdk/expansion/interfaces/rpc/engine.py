from typing import Optional, NamedTuple

import expansion.api as api
from expansion.transport import IOTerminal, Channel
from expansion.types.geometry import Vector

import expansion.utils as utils


class Specification(NamedTuple):
    max_thrust: int


def _thrust_from_protobuf(thrust) -> Vector:
    if not thrust.thrust:
        return Vector(0, 0)
    return Vector(x=thrust.x, y=thrust.y).set_length(thrust.thrust)


class EngineI(IOTerminal):

    def __init__(self, name: Optional[str] = None):
        super().__init__(name=name or utils.generate_name(EngineI))
        self.specification: Optional[Specification] = None

    @Channel.return_on_close(None)
    async def get_specification(self, timeout: float = 0.5, reset_cached=False)\
            -> Optional[Specification]:
        if reset_cached:
            self.specification = None
        if self.specification:
            return self.specification
        request = api.Message()
        request.engine.specification_req = True
        if not self.send(message=request):
            return None
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return None
        spec = api.get_message_field(response, ["engine", "specification"])
        if not spec:
            return None
        self.specification = Specification(max_thrust=spec.max_thrust)
        return self.specification

    @Channel.return_on_close(None)
    async def get_thrust(self, timeout: float = 0.5) -> Optional[Vector]:
        """Return current engine thrust"""
        request = api.Message()
        request.engine.thrust_req = True
        if not self.send(message=request):
            return None
        return await self.wait_thrust(timeout=timeout)

    @Channel.return_on_close(None)
    async def wait_thrust(self, timeout: float = 0.5) -> Optional[Vector]:
        """Wait for a CurrentThrust indication on this session"""
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return None
        thrust = api.get_message_field(response, ["engine", "thrust"])
        if not thrust:
            return None
        return _thrust_from_protobuf(thrust)

    @Channel.return_on_close(None)
    async def monitor(self, timeout: float = 2.0) -> Optional[Vector]:
        """Start a monitoring session and return the current thrust snapshot.
        The only way to stop monitoring is to close the session.
        """
        request = api.Message()
        request.engine.monitor = True
        if not self.send(message=request):
            return None
        return await self.wait_thrust(timeout=timeout)

    @Channel.return_on_close(False)
    async def set_thrust(self, thrust: Vector, at: int = 0, duration_ms: int = 0) -> bool:
        """Set engine thrust to the specified 'thrust' for the
        specified 'duration_ms' milliseconds. If 'duration_ms' is 0, then
        the thrust will be set until another command.
        Function doesn't await any acknowledgement or response.
        Return true if a request has been sent
        """
        request = api.Message()
        if at:
            request.timestamp = at
        thrust_req = request.engine.change_thrust
        thrust_req.x = thrust.x
        thrust_req.y = thrust.y
        thrust_req.thrust = int(thrust.abs())
        thrust_req.duration_ms = duration_ms
        return self.channel.send(message=request)
