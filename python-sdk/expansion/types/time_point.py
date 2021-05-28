from typing import Optional
import time


class TimePoint:
    """TimePoint helps to follow current server time

    TimePoint sotres two timestamps: a 'server_time' and a 'local_time', that
    correspond to the stored 'server_time'. So it can predict server's time
    using a simple approach:
    predicted_server_time = server_tim + (now() - local_time)

    Note that time prediction is not monotonic! Use the 'monotonic()' call
    if you really need monotonic behavior.
    """

    def __init__(self, ingame_time: int, static: bool = False):
        assert ingame_time is not None
        self._server_time: int = ingame_time
        self._local_time: Optional[float] = None if static else time.monotonic()

    def update(self, server_time: int):
        self._server_time: int = server_time
        if self._local_time is not None:
            self._local_time = time.monotonic()

    def __str__(self) -> str:
        if self._local_time is not None:
            return f"Server: {self._server_time}, Local: {self._local_time}"
        else:
            return f"{self._server_time}"

    def usec(self) -> int:
        return self._server_time

    def sec(self) -> float:
        return self._server_time / 10**6

    def predict_usec(self) -> int:
        """Return predited current time in microseconds

        Note: this function doesn't guarantee monotonic behavior!
        If you need monotonic time, use the `monotonic()` instead
        """
        assert self._local_time is not None
        dt = int((time.monotonic() - self._local_time) * 1000000)
        return self._server_time + dt

    def dt_sec(self) -> float:
        """Return seconds since the timepoint has been created/updated"""
        assert self._local_time is not None
        return time.monotonic() - self._local_time
