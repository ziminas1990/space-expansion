import asyncio
from typing import Optional, ContextManager
import contextlib

from .commutator import Commutator
from .factory import module_factory
import expansion.interfaces.rpc as rpc
import expansion.procedures as procedures


class RootCommutator(Commutator):
    def __init__(self, name: str = "Root"):
        super().__init__(
            connection_factory=self.__open_tunnel_fake,
            modules_factory=module_factory,
            name=name)
        self.mutex = asyncio.Lock()
        self.remote: Optional[rpc.CommutatorI] = None

    async def login(self,
                    server_ip: str, login_port: int,
                    login: str, password: str,
                    local_ip: str, local_port: int) -> Optional[str]:
        assert self.remote is None, "Already logged in!"
        self.remote, error = await procedures.login(
            server_ip=server_ip,
            login_port=login_port,
            login=login,
            password=password,
            local_ip=local_ip,
            local_port=local_port)
        if error is not None:
            error = f"Failed to login: {error}"
            self.logger.warning(error)
            return error
        assert self.remote is not None
        return None

    # Overrides BaseModule's implementation
    @contextlib.asynccontextmanager
    async def _lock_channel(self) -> ContextManager[rpc.CommutatorI]:
        """This function will always return the same channel, because
        root commutator can't spawn a new channels to the remote side. So
        this function will block until the channel is used by someone else"""
        try:
            await self.mutex.acquire()
            assert self.remote is not None, \
                "Not connected to the remote side. Are you logged in?"
            yield self.remote
        finally:
            self.mutex.release()

    @staticmethod
    async def __open_tunnel_fake():
        # This function should be never called, because the '_lock_channel'
        # call is overridden!
        assert False
