from typing import Optional, NamedTuple, List

import expansion.api as api
from expansion.transport import IOTerminal, Channel
import expansion.utils as utils


class Score(NamedTuple):
    player: str
    score: int


class GameOver(NamedTuple):
    leaders: List[Score]


def _game_over_from_protobuf(report) -> GameOver:
    return GameOver(leaders=[
        Score(player=item.player, score=item.score)
        for item in report.leaders
    ])


class GameI(IOTerminal):

    def __init__(self, name: Optional[str] = None):
        super().__init__(name=name or utils.generate_name(GameI))

    @Channel.return_on_close(False)
    async def monitor(self) -> bool:
        """Start monitoring on this session. The only way to stop
        monitoring is to close the session.
        """
        request = api.Message()
        request.game.monitor = True
        return self.send(message=request)

    @Channel.return_on_close(None)
    async def wait_game_over(self, timeout: float = 0) -> Optional[GameOver]:
        """Wait for a game_over_report on this session."""
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return None
        report = api.get_message_field(response, ["game", "game_over_report"])
        if not report:
            return None
        return _game_over_from_protobuf(report)
