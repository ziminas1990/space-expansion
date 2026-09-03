import math
from typing import List, NamedTuple, Optional, Tuple

import expansion.interfaces.rpc as rpc
from expansion.modules.engine import Engine
from expansion.modules.ship import Ship
from expansion.types import Position, Status, TimePoint, Vector


IDLE = 1e-9
BURN_MIN_SEC = 1e-12
AXIS_TIME_MATCH_SEC = 1e-4
SPLIT_ITERATIONS = 40
SEARCH_CYCLES = 32


def accelerate(start: Position, acc: Vector, t_sec: float) -> Position:
    """Predict a position after a constant acceleration for ``t_sec``."""
    dv = acc * t_sec
    ds = (start.velocity + dv / 2) * t_sec
    end_at = t_sec * 10 ** 6 + (start.timestamp.usec() if start.timestamp else 0)
    return Position(
        x=start.x + ds.x,
        y=start.y + ds.y,
        velocity=start.velocity + dv,
        timestamp=TimePoint(round(end_at), static=True),
    )


class Maneuver(NamedTuple):
    at: int
    duration: int
    acc: Vector

    def ends_at(self) -> int:
        return self.at + self.duration

    def apply_to(self, position: Position) -> Position:
        if position.timestamp:
            position = position.predict(self.at)
        return accelerate(position, self.acc, self.duration / 10 ** 6)

    def partially_apply_to(self, position: Position, duration_usec: int) -> Position:
        return accelerate(position, self.acc, duration_usec / 10 ** 6)


def squash_maneuvers(maneuvers: List[Maneuver]) -> List[Maneuver]:
    result: List[Maneuver] = []
    for maneuver in maneuvers:
        previous = result[-1] if result else None
        if (
            previous is not None
            and previous.acc.codirected(maneuver.acc)
            and (previous.acc - maneuver.acc).abs() < 0.00001
        ):
            result[-1] = Maneuver(
                at=previous.at,
                duration=previous.duration + maneuver.duration,
                acc=previous.acc,
            )
        else:
            result.append(maneuver)
    return result


class FlightPlan(NamedTuple):
    maneuvers: List[Maneuver]

    def time_points(self) -> List[int]:
        points: List[int] = []
        for maneuver in self.maneuvers:
            if not points or points[-1] < maneuver.at:
                points.extend([maneuver.at, maneuver.ends_at()])
            else:
                points.append(maneuver.ends_at())
        return points

    def acceleration_at(self, at_us: int) -> Vector:
        for maneuver in self.maneuvers:
            if at_us < maneuver.at:
                return Vector(0, 0)
            if maneuver.at <= at_us < maneuver.ends_at():
                return maneuver.acc
        return Vector(0, 0)

    def max_acceleration(self) -> float:
        return max((maneuver.acc.abs() for maneuver in self.maneuvers), default=0)

    def duration_usec(self) -> int:
        return self.ends_at() - self.starts_at()

    def duration_sec(self) -> float:
        return self.duration_usec() / 10 ** 6

    def delta_v(self) -> float:
        return sum(
            maneuver.acc.abs() * maneuver.duration / 10 ** 6
            for maneuver in self.maneuvers
        )

    def starts_at(self) -> int:
        return self.maneuvers[0].at if self.maneuvers else 0

    def ends_at(self) -> int:
        return self.maneuvers[-1].ends_at() if self.maneuvers else 0

    @staticmethod
    def merge(plans: List["FlightPlan"], squash: bool = True) -> "FlightPlan":
        time_points = sorted({point for plan in plans for point in plan.time_points()})
        maneuvers: List[Maneuver] = []
        for begin, end in zip(time_points, time_points[1:]):
            acc = Vector(0, 0)
            for plan in plans:
                acc += plan.acceleration_at(begin)
            maneuvers.append(Maneuver(begin, end - begin, acc))
        return FlightPlan(squash_maneuvers(maneuvers) if squash else maneuvers)

    def apply_to(self, position: Position) -> Position:
        for maneuver in self.maneuvers:
            position = maneuver.apply_to(position)
        return position

    def partially_apply_to(self, position: Position, duration_usec: int) -> Position:
        remaining = duration_usec
        for maneuver in self.maneuvers:
            if position.timestamp and position.timestamp.usec() < maneuver.ends_at():
                dt = min(maneuver.ends_at() - position.timestamp.usec(), remaining)
                position = maneuver.partially_apply_to(position, dt)
                remaining -= dt
                if remaining <= 0:
                    break
        return position

    def build_path(self, position: Position, step_ms: int = 1000) -> List[Position]:
        path: List[Position] = []
        while position.timestamp and position.timestamp.usec() < self.ends_at():
            position = self.partially_apply_to(position, step_ms * 1000)
            path.append(position)
        return path


class _AxisBurn(NamedTuple):
    duration_sec: float
    acc: float


def _usec(position: Position) -> int:
    return position.timestamp.usec() if position.timestamp else 0


def _sign(value: float) -> int:
    return -1 if value < 0 else 1 if value > 0 else 0


def _axis_idle(position: float, velocity: float) -> bool:
    return abs(position) <= IDLE and abs(velocity) <= IDLE


def _burns_duration(burns: List[_AxisBurn]) -> float:
    return sum(burn.duration_sec for burn in burns)


def _two_bang(position: float, velocity: float, acc: float) -> Optional[List[_AxisBurn]]:
    delta = 0.5 * (velocity / acc) ** 2 - position / acc
    if delta < 0:
        return None
    t2 = math.sqrt(delta)
    t1 = t2 - velocity / acc
    if t1 < -BURN_MIN_SEC or t2 < -BURN_MIN_SEC:
        return None
    burns = []
    if t1 > BURN_MIN_SEC:
        burns.append(_AxisBurn(t1, acc))
    if t2 > BURN_MIN_SEC:
        burns.append(_AxisBurn(t2, -acc))
    return burns


def _stop_at_zero_1d(
    position: float, velocity: float, amax: float
) -> Optional[List[_AxisBurn]]:
    if _axis_idle(position, velocity):
        return []
    if not amax > 0 or not math.isfinite(amax):
        return None

    stop_offset = velocity * abs(velocity) / (2 * amax)
    if abs(position + stop_offset) <= IDLE:
        duration = abs(velocity) / amax
        return [] if duration <= BURN_MIN_SEC else [
            _AxisBurn(duration, -_sign(velocity) * amax)
        ]

    candidates = [
        burns
        for acc in (amax, -amax)
        if (burns := _two_bang(position, velocity, acc))
    ]
    return min(candidates, key=_burns_duration) if candidates else None


def _axis_plan(burns: List[_AxisBurn], axis: int, now: int) -> FlightPlan:
    maneuvers: List[Maneuver] = []
    at = now
    for burn in burns:
        duration = round(burn.duration_sec * 10 ** 6)
        if duration <= 0:
            continue
        acc = Vector(burn.acc, 0) if axis == 0 else Vector(0, burn.acc)
        maneuvers.append(Maneuver(at, duration, acc))
        at += duration
    return FlightPlan(maneuvers)


def _split_amax(
    x: float, vx: float, y: float, vy: float, amax: float
) -> Optional[Tuple[float, float]]:
    x_idle = _axis_idle(x, vx)
    y_idle = _axis_idle(y, vy)
    if x_idle and y_idle:
        return amax, 0
    if x_idle:
        return 0, amax
    if y_idle:
        return amax, 0

    left, right = 0.0, math.pi / 2
    best: Optional[Tuple[float, float]] = None
    best_error = math.inf
    for _ in range(SPLIT_ITERATIONS):
        alpha = (left + right) / 2
        ax, ay = amax * math.cos(alpha), amax * math.sin(alpha)
        x_burns = _stop_at_zero_1d(x, vx, ax)
        y_burns = _stop_at_zero_1d(y, vy, ay)
        if x_burns is None or y_burns is None:
            return None
        error = _burns_duration(x_burns) - _burns_duration(y_burns)
        if abs(error) < best_error:
            best_error, best = abs(error), (ax, ay)
        if abs(error) < AXIS_TIME_MATCH_SEC:
            return ax, ay
        if error < 0:
            left = alpha
        else:
            right = alpha
    return best


def _stop_at_origin(position: Position, amax: float) -> Optional[FlightPlan]:
    if not amax > 0 or not math.isfinite(amax):
        if _axis_idle(position.x, position.velocity.x) and _axis_idle(
            position.y, position.velocity.y
        ):
            return FlightPlan([])
        return None
    split = _split_amax(
        position.x, position.velocity.x, position.y, position.velocity.y, amax
    )
    if split is None:
        return None
    ax, ay = split
    x_burns = _stop_at_zero_1d(position.x, position.velocity.x, ax)
    y_burns = _stop_at_zero_1d(position.y, position.velocity.y, ay)
    if x_burns is None or y_burns is None:
        return None
    now = _usec(position)
    return FlightPlan.merge([
        _axis_plan(x_burns, 0, now),
        _axis_plan(y_burns, 1, now),
    ])


def _relative_to_target(position: Position, target: Position) -> Position:
    target_now = target.predict(_usec(position)) if target.timestamp else target
    return Position(
        position.x - target_now.x,
        position.y - target_now.y,
        position.velocity - target_now.velocity,
        position.timestamp,
    )


def _plan_to_target(
    position: Position, target: Position, amax: float
) -> Optional[FlightPlan]:
    return _stop_at_origin(_relative_to_target(position, target), amax)


def prepare_flight_plan(
    position: Position, target: Position, amax: float
) -> Optional[FlightPlan]:
    """Build a full-thrust ballistic intercept plan."""
    return _plan_to_target(position, target, amax)


def approach_to_plan(
    position: Position, target: Position, amax: float
) -> Optional[FlightPlan]:
    """Build a full-thrust ballistic intercept plan."""
    return _plan_to_target(position, target, amax)


def prepare_flight_plan_in_time(
    position: Position,
    target: Position,
    amax: float,
    tmin: float,
    tmax: float,
) -> Tuple[Status, Optional[FlightPlan]]:
    fastest = _plan_to_target(position, target, amax)
    if fastest is None:
        return Status.fail("failed to build a plan"), None
    if fastest.duration_sec() > tmax:
        return Status.fail("full thrust exceeds tmax"), None
    if fastest.duration_sec() >= tmin:
        return Status.ok(), fastest

    left, right = 0.0, amax
    for _ in range(SEARCH_CYCLES):
        candidate = (left + right) / 2
        plan = _plan_to_target(position, target, candidate)
        if plan is None:
            left = candidate
            continue
        duration = plan.duration_sec()
        if tmin <= duration <= tmax:
            return Status.ok(), plan
        if duration < tmin:
            right = candidate
        else:
            left = candidate
    return Status.fail("time window search did not converge"), None


def prepare_flight_plan_in_delta_v(
    position: Position,
    target: Position,
    amax: float,
    dvmin: float,
    dvmax: float,
) -> Tuple[Status, Optional[FlightPlan]]:
    fastest = _plan_to_target(position, target, amax)
    if fastest is None:
        return Status.fail("failed to build a plan"), None
    fastest_dv = fastest.delta_v()
    if fastest_dv < dvmin:
        return Status.fail("full thrust below dvmin"), None
    if fastest_dv <= dvmax:
        return Status.ok(), fastest

    left, right = 0.0, amax
    for _ in range(SEARCH_CYCLES):
        candidate = (left + right) / 2
        plan = _plan_to_target(position, target, candidate)
        if plan is None:
            left = candidate
            continue
        delta_v = plan.delta_v()
        if dvmin <= delta_v <= dvmax:
            return Status.ok(), plan
        if delta_v > dvmax:
            right = candidate
        else:
            left = candidate
    return Status.fail("delta-v window search did not converge"), None


async def follow_flight_plan(
    ship: Ship,
    engine: Engine,
    plan: FlightPlan,
    system_clock: rpc.SystemClockI,
) -> bool:
    for maneuver in plan.maneuvers:
        await system_clock.wait_until(time=maneuver.at - 25000)
        ship_state = await ship.get_state()
        if ship_state is None:
            return False
        thrust = ship_state.weight * maneuver.acc.abs()
        if not await engine.set_thrust(
            thrust=maneuver.acc.set_length(thrust, inplace=False),
            at=maneuver.at,
            duration_ms=round(maneuver.duration / 1000),
        ):
            return False
    if plan.maneuvers:
        await system_clock.wait_until(time=plan.ends_at())
    return True
