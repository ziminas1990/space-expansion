import abc
import time
from typing import Tuple, Optional, Any
from expansion.transport import Channel, Terminal


class Endpoint(Channel, Terminal, abc.ABC):

    @abc.abstractmethod
    async def wait_message(self, timeout: float = 1.0) -> Tuple[Optional[Any], Optional[int]]:
        """Await for a message on the internal queue for not more than the
        specified 'timeout' seconds. Return a message and a optional timestamp, when
        the message was sent."""
        pass

    async def wait_exact(self, message: str, timeout: float = 1.0) \
            -> Tuple[Optional[Any], Optional[int]]:
        """Await for the specified 'message' but not more than 'timeout' seconds.
        Ignore all other received messages. Return expected message and timestamp or
        None"""
        while timeout > 0:
            start_at = time.monotonic()
            received_msg, timestemp = await self.wait_message(timeout)
            expected_msg = get_message_field(received_msg, message)
            if expected_msg:
                return expected_msg, timestemp
            # Got unexpected message. Just ignoring it
            timeout -= time.monotonic() - start_at
        return None, None
