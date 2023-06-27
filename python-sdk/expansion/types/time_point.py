from typing import Optional
import time


class TimePoint:
    """TimePoint helps to follow current server time

    TimePoint stores two timestamps: a 'remote_time' and a 'local_time', that
    correspond to the stored 'remote_time'. So it can predict server's time
    using a simple approach:
    predicted_remote_time = remote_time + (now() - local_time)

    Note that time prediction is not monotonic! Use the 'monotonic()' call
    if you really need monotonic behavior.
    """

    def __init__(self, remote_time: int, static: bool = False):
        assert remote_time is not None
        self._remote_time_us: int = remote_time
        self._local_time_ns: Optional[int] = None if static else time.monotonic_ns()

    def update(self, server_time: int):
        self._remote_time_us: int = server_time
        if self._local_time_ns is not None:
            self._local_time_ns = time.monotonic_ns()

    def __str__(self) -> str:
        if self._local_time_ns is not None:
            return f"Server: {self._remote_time_us}, Local: {self._local_time_ns}"
        else:
            return f"{self._remote_time_us}"

    def usec(self) -> int:
        return self._remote_time_us

    def sec(self) -> float:
        return self._remote_time_us / 10**6

    def predict_usec(self) -> int:
        """Return predicted current time in microseconds"""
        assert self._local_time_ns is not None
        dt_us = int((time.monotonic_ns() - self._local_time_ns) / 1000)
        return self._remote_time_us + dt_us

    def predict_sec(self) -> float:
        """Return predicted current time in seconds"""
        assert self._local_time_ns is not None
        dt_us = (time.monotonic_ns() - self._local_time_ns) / 1000
        return (self._remote_time_us + dt_us) / 10 ** 6

    def dt_sec(self) -> float:
        """Return seconds since the timepoint has been created/updated"""
        assert self._local_time_ns is not None
        return (time.monotonic_ns() - self._local_time_ns) / 10 ** 9

    def more_recent(self, other: "TimePoint") -> bool:
        if self._local_time_ns is None:
            if other._local_time_ns is None:
                # Both timepoints are static and can be compared
                return self._remote_time_us > other._remote_time_us
        else:
            # this timepoint is not static
            # non-static (live) timepoint has priority against static
            return other._local_time_ns is None or self._remote_time_us > other._remote_time_us
