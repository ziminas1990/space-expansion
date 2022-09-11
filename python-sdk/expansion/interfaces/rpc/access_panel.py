from typing import Optional

import expansion.api as api
from expansion.transport import Channel, IOTerminal
from expansion.api.utils import get_message_field


class AccessPanelI:

    def __init__(self):
        self.socket: IOTerminal = IOTerminal()

    def attach_to_channel(self, channel: Channel):
        self.socket.wrap_channel(channel)

    async def login(self, login: str, password: str) -> (int, int, Optional[str]):
        """Try to open privileged session as user with the
        specified 'login' and 'password'.
        Return tuple (port, session_id, error_string)"""
        message = api.Message()
        login_req = message.accessPanel.login
        login_req.login = login
        login_req.password = password
        self.socket.send(message)

        response, _ = await self.socket.wait_message()
        if not response:
            return 0, 0, "response timeout"

        access_granted = get_message_field(
            response,
            ["accessPanel", "access_granted"])
        if access_granted:
            return access_granted.port, access_granted.root_session_id, None

        error: Optional[str] = get_message_field(
            response,
            ["accessPanel", "access_rejected"])
        return 0, 0, error if error is not None else "Unexpected response"
