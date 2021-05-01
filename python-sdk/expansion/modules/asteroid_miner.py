from typing import Optional, Callable, Awaitable

from expansion.interfaces.rpc import AsteroidMinerI, AsteroidMinerSpec
from expansion import utils
from .base_module import BaseModule, TunnelFactory
from expansion.types import ResourceType


class AsteroidMiner(BaseModule):

    Status = AsteroidMinerI.Status

    def __init__(self,
                 tunnel_factory: TunnelFactory,
                 name: Optional[str] = None):
        super().__init__(tunnel_factory=tunnel_factory,
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
        async with self.rent_session(AsteroidMinerI) as channel:
            self.specification = await channel.get_specification(timeout)
        return self.specification

    async def bind_to_cargo(self, cargo_name: str, timeout: float = 0.5) -> Status:
        """Bind miner to the container with the specified 'cargo_name'"""
        async with self.rent_session(AsteroidMinerI) as channel:
            status = await channel.bind_to_cargo(cargo_name, timeout)
            if status == AsteroidMinerI.Status.SUCCESS:
                self.cargo_name = cargo_name
            return status

    async def start_mining(self,
                           asteroid_id: int,
                           progress_cb: AsteroidMinerI.MiningReportCallback,
                           timeout: float = 0.5) -> Status:
        """Start mining asteroid with the specified 'asteroid_id'.
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
        async with self.rent_session(AsteroidMinerI) as channel:
            return await channel.start_mining(asteroid_id, progress_cb, timeout)

    async def stop_mining(self, timeout: float = 0.5) -> Status:
        """Stop the mining process"""
        async with self.rent_session(AsteroidMinerI) as channel:
            return await channel.stop_mining(timeout)
