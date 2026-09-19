from typing import Optional, AsyncIterable, TYPE_CHECKING

from expansion.interfaces.rpc import GameI, GameOver
import expansion.utils as utils
from .base_module import BaseModule, ModuleType, TunnelFactory

if TYPE_CHECKING:
    from expansion.modules import Commutator


class Game(BaseModule):

    def __init__(self,
                 tunnel_factory: TunnelFactory,
                 name: Optional[str] = None):
        super().__init__(tunnel_factory=tunnel_factory,
                         name=name or utils.generate_name(Game))

    @BaseModule.use_session_for_generators(
        terminal_type=GameI,
        return_on_unreachable=None
    )
    async def monitor(self,
                      session: Optional[GameI] = None) \
            -> AsyncIterable[Optional[GameOver]]:
        """Yield game updates on a dedicated session. Yields None on timeout.
        Close the generator (or the session) to stop monitoring.
        """
        assert session is not None
        if not await session.monitor():
            yield None
            return
        while True:
            yield await session.wait_game_over(timeout=0)

    @staticmethod
    def find(commutator: "Commutator") -> Optional["Game"]:
        return BaseModule._get_any(
            commutator=commutator,
            type=ModuleType.GAME
        )
