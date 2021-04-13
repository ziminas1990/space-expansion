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

    def __init__(self, ingame_time: int):
        assert ingame_time is not None
        self._server_time: int = 0
        self._local_time: float = 0
        self._last_monotonic: int = 0
        self.update(ingame_time)

    def update(self, server_time: int):
        self._server_time: int = server_time
        self._local_time: float = time.monotonic()

    def now(self, predict: bool = True) -> int:
        """Return approximate current time in microseconds

        if 'predict' is True than return a predicted time of the server,
        otherwise return last received time.
        Note: if predict is True than this function doesn't guarantee
        monotonic behavior! If you need monotonic time, use the
        `monotonic()` instead
        """
        if predict:
            dt = int((time.monotonic() - self._local_time) * 1000000)
            return self._server_time + dt
        else:
            return self._server_time

    def monotonic(self, predict: bool = True) -> int:
        """Return approximate current time in microseconds

        if 'predict' is True than return a predicted time of the server,
        otherwise return last received time."""
        now = self.now(predict)
        if now > self._last_monotonic:
            self._last_monotonic = now
        return self._last_monotonic

    def dt_sec(self) -> float:
        """Return seconds since the timepoint has been created/updated"""
        return time.monotonic() - self._local_time
