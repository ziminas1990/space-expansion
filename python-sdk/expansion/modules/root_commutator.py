import asyncio
from typing import Optional, ContextManager, Type
import contextlib

from .commutator import Commutator
from .factory import module_factory
import expansion.interfaces.rpc as rpc
import expansion.procedures as procedures


class RootCommutator(Commutator):
    def __init__(self, name: str = "Root"):
        super().__init__(
            tunnel_factory=self.__open_tunnel_fake,
            modules_factory=module_factory,
            name=name)
        self.mutex = asyncio.Lock()
        self.remote: Optional[rpc.CommutatorI] = None

    # Overrides Commutator implementation
    async def init(self) -> bool:
        return await super().init(session=self.remote)

    # Overrides Commutator implementation
    async def _open_tunnel(self, slot_id: int):
        async with self.mutex:
            status, tunnel = await self.remote.open_tunnel(port=slot_id)
            return tunnel, None if status.is_success() else str(status)

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

    @staticmethod
    async def __open_tunnel_fake():
        # This function should never be called
        assert False
