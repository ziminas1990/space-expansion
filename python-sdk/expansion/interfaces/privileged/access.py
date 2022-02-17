from typing import Optional

from expansion.transport import IOTerminal
import expansion.api as api
from expansion.api.utils import get_message_field


class Access(IOTerminal):
    def __init__(self, name: str, *args, **kwargs):
        super(Access, self).__init__(name=name, *args, **kwargs)

    async def login(self, login: str, password: str) -> (bool, Optional[int]):
        """Try to open privileged session as user with the
        specified 'login' and 'password'. Return tuple (status, token)"""
        message = api.admin.Message()
        login_req = message.access.login
        login_req.login = login
        login_req.password = password
        self.send(message)

        response, _ = await self.wait_message()
        token: Optional[int] = get_message_field(response, ["access", "success"])
        return token is not None, token
