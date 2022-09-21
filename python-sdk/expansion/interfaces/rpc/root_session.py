from typing import Optional, Tuple
import logging

from expansion.transport import IOTerminal, Channel
import expansion.api as api

import expansion.utils as utils


class RootSession(IOTerminal):

    def __init__(self, name: Optional[str] = None, trace_mode: bool = False):
        super().__init__(name=name, trace_mode=trace_mode)
        if name is None:
            name = utils.generate_name(RootSession)
        self.logger = logging.getLogger(name)

    @Channel.return_on_close(None)
    async def open_commutator_session(self, timeout: float = 0.1) \
            -> Tuple[Optional[int], Optional[str]]:
        request = api.Message()
        request.root_session.new_commutator_session = True
        if not self.send(message=request):
            return None, "Can't send request"
        return await self.wait_commutator_session_id(timeout)

    @Channel.return_on_close(None)
    async def wait_commutator_session_id(self, timeout: float = 0.5)\
            -> Tuple[Optional[int], Optional[str]]:
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return None, "response timeout"
        session_id = api.get_message_field(
            response, ["root_session", "commutator_session"])
        if session_id is None:
            return None, "got an unexpected response"
        return session_id, None

    # Close the root session.
    # Note that once root session is closed, the connection and all
    # it's sessions will be closed as well.
    def close(self):
        request = api.Message()
        request.session.close = True
        self.send(message=request)