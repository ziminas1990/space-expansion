from typing import TypeVar, Optional, List, Union, Any

T = TypeVar('T')


class WellKnownId:
    SUCCESS = "Status.SUCCESS"
    UNKNOWN = "Status.UNKNOWN"
    CANCELLED = "Status.CANCELLED"
    CHANNEL_IS_CLOSED = "Status.CHANNEL_IS_CLOSED"
    UNREACHABLE = "Status.UNREACHABLE"
    TIMEOUT = "Status.TIMEOUT"
    FAILED_TO_SEND_MSG = "Status.FAILED_TO_SEND_MSG"
    UNEXPECTED_MESSAGE = "Status.UNEXPECTED_MESSAGE"


class Status:

    @staticmethod
    def fail(what: str, status_id: str = WellKnownId.UNKNOWN):
        return Status(is_fail=True, details=what, status_id=status_id)

    @staticmethod
    def ok(details: Optional[str] = None, status_id: str = WellKnownId.SUCCESS):
        return Status(is_fail=False, details=details, status_id=status_id)

    def __init__(self, *,
                 is_fail: bool,
                 status_id: str,
                 details: Optional[str] = None):
        self.is_fail = is_fail
        self.details: List[str] = []
        if details:
            self.details.append(details)
        self.status_id: str = status_id

    def __bool__(self) -> bool:
        return not self.is_fail

    def is_ok(self) -> bool:
        return not self.is_fail

    def __str__(self) -> str:
        if len(self.details) > 0:
            return ": ".join(reversed(self.details.reverse)) + f" (${self.status_id})"
        return "ok" if self.is_ok() else "unknown error" + f" (${self.status_id})"

    def __eq__(self, other: Union["Status", str]) -> bool:
        if isinstance(other, Status):
            return self.status_id == other.status_id and \
                   self.status_id != WellKnownId.UNKNOWN
        elif isinstance(other, str):
            return self.status_id == other
        else:
            return False

    def wrap_fail(self, what: str) -> "Status":
        assert not self.is_ok()
        self.details.append(what)
        return self

    # Well known statuses
    @staticmethod
    def channel_is_closed() -> "Status":
        return Status.fail("channel is closed", WellKnownId.CHANNEL_IS_CLOSED)

    @staticmethod
    def response_timeout() -> "Status":
        return Status.fail("response timeout", WellKnownId.TIMEOUT)

    @staticmethod
    def unexpected_message(message: Optional[Any] = None) -> "Status":
        return Status.fail(
                f"unexpected message: {message}" if message
                else "unexpected message",
                WellKnownId.UNEXPECTED_MESSAGE)

    @staticmethod
    def failed_to_send(message: Optional[Any] = None) -> "Status":
        return Status.fail(
            f"failed to send message: {message}" if message
            else "failed to send message",
            WellKnownId.FAILED_TO_SEND_MSG)

    @staticmethod
    def unreachable() -> "Status":
        return Status.fail("destination is unreachable", WellKnownId.UNREACHABLE)

    @staticmethod
    def cancelled() -> "Status":
        return Status.fail("operation cancelled", WellKnownId.CANCELLED)

    @staticmethod
    def unknown() -> "Status":
        return Status.fail("unknown error", WellKnownId.UNKNOWN)
