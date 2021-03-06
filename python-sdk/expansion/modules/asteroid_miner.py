from typing import Optional, Callable, Awaitable

from expansion.interfaces.public import AsteroidMinerI, AsteroidMinerSpec
from expansion import utils
from .base_module import BaseModule
from expansion.types import ResourceType


class AsteroidMiner(BaseModule):

    Status = AsteroidMinerI.Status

    def __init__(self,
                 connection_factory: Callable[[], Awaitable[AsteroidMinerI]],
                 name: Optional[str] = None):
        super().__init__(connection_factory=connection_factory,
                         logging_name=name or utils.generate_name(AsteroidMiner))
        self.specification: Optional[AsteroidMinerSpec] = None
        self.cargo_name: Optional[str] = None
        # A name of the resource container, to which the miner is attached

    async def get_specification(self, timeout: float = 0.5, reset_cached=False) \
            -> Optional[AsteroidMinerSpec]:
        if reset_cached:
            self.specification = None
        if self.specification:
            return self.specification
        async with self._lock_channel() as channel:
            assert isinstance(channel, AsteroidMinerI)  # sort of type hinting
            self.specification = await channel.get_specification(timeout)
        return self.specification

    async def bind_to_cargo(self, cargo_name: str, timeout: float = 0.5) -> Status:
        """Bind miner to the container with the specified 'cargo_name'"""
        async with self._lock_channel() as channel:
            assert isinstance(channel, AsteroidMinerI)  # sort of type hinting
            status = await channel.bind_to_cargo(cargo_name, timeout)
            if status == AsteroidMinerI.Status.SUCCESS:
                self.cargo_name = cargo_name
            return status

    async def start_mining(self,
                           asteroid_id: int,
                           resource: ResourceType,
                           progress_cb: AsteroidMinerI.MiningReportCallback,
                           timeout: float = 0.5) -> Status:
        """Start mining asteroid with the specified 'asteroid_id' for the
        specified 'resource'. Only the specified resource will be retrieved.
        The specified 'progress_cb' will be called in the following cases:
        1. the mining report is received from server - in this case a
           'SUCCESS' status will be passed as the first argument, and a total
           retrieved resources (since last call) will be passed as the second
           argument. If callback returns False, then mining process will be
           interrupted, otherwise it will be resumed.
        3. some error occurred during mining - in this case an error code (status)
           will be passed as the first argument, and 'None' as the second argument;
           in this case a value, returned by callback, will be ignored and the mining
           process will be interrupted.
        """
        async with self._lock_channel() as channel:
            assert isinstance(channel, AsteroidMinerI)  # sort of type hinting
            return await channel.start_mining(asteroid_id, resource, progress_cb, timeout)

    async def stop_mining(self, timeout: float = 0.5) -> Status:
        """Stop the mining process"""
        async with self._lock_channel() as channel:
            assert isinstance(channel, AsteroidMinerI)  # sort of type hinting
            return await channel.stop_mining(timeout)
