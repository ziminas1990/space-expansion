import copy
import random
from typing import Callable, Awaitable, NamedTuple, List, Tuple, Optional, Iterator
import math
import asyncio

from expansion.types import Position, Vector, TimePoint
from expansion.modules.ship import Ship
from expansion.modules.engine import Engine, EngineSpec
import expansion.interfaces.rpc as rpc
import expansion.modules as modules


def accelerate(start: Position, acc: Vector, t_sec: float) -> Position:
    """Return a result of acceleration with the specified 'acc' from the
    specified 'start' position during the specified 't_sec' seconds"""
    dv = acc * t_sec
    ds = (start.velocity + dv / 2) * t_sec
    end_at = t_sec * 10 ** 6 + (start.timestamp.usec() if start.timestamp else 0)
    return Position(x = start.x + ds.x,
                    y = start.y + ds.y,
                    velocity = start.velocity + dv,
                    timestamp = TimePoint(round(end_at), static=True))


class Maneuver(NamedTuple):
    at: int        # When the specified acceleration should be set
    duration: int  # How long acceleration should last
    acc: Vector    # An acceleration that should be set

    def __cmp__(self, other: "Maneuver"):
        return self.at - other.at

    def ends_at(self) -> int:
        return self.at + self.duration

    def apply_to(self, position: Position) -> Position:
        """Predict a new position of the object if this maneuver is applied to it."""
        if position.timestamp:
            assert position.timestamp.usec() <= self.at
            position = position.predict(self.at)

        dt = self.duration / 10**6
        return accelerate(position, self.acc, dt)

    def partially_apply_to(self, position: Position, duration_usec: int) -> Position:
        maneuver_ends_at = self.at + self.duration
        assert self.at <= position.timestamp.usec()
        assert position.timestamp.usec() + duration_usec <= maneuver_ends_at
        return accelerate(position, self.acc, duration_usec / 10**6)


def squash_maneuvers(maneuvers: List[Maneuver]):
    result: List[Maneuver] = []
    for maneuver in maneuvers:
        doSquash = result \
                   and result[-1].acc.codirected(maneuver.acc) \
                   and (result[-1].acc - maneuver.acc).abs() < 0.00001
        if doSquash:
            previous = result[-1]
            result[-1] = Maneuver(
                at=previous.at,
                duration=previous.duration + maneuver.duration,
                acc=previous.acc)
        else:
            result.append(maneuver)
    return result


class FlightPlan(NamedTuple):
    maneuvers: List[Maneuver]

    def time_points(self) -> List[int]:
        # Return timepoints where acceleration should be changed
        points: List[int] = []
        for maneuver in self.maneuvers:
            if not points or points[-1] < maneuver.at:
                points.extend([maneuver.at, maneuver.at + maneuver.duration])
            else:
                points.append(maneuver.at + maneuver.duration)
        return points

    def acceleration_at(self, at_us: int) -> Vector:
        for maneuver in self.maneuvers:
            if at_us < maneuver.at:
                return Vector(0, 0)
            if maneuver.at <= at_us < maneuver.at + maneuver.duration:
                return maneuver.acc
        return Vector(0, 0)

    def max_acceleration(self) -> float:
        max = 0
        for maneuver in self.maneuvers:
            if maneuver.acc.abs() > max:
                max = maneuver.acc.abs()
        return max

    def duration_usec(self) -> int:
        if not self.maneuvers:
            return 0
        begin = self.maneuvers[0].at
        end = self.maneuvers[-1].at + self.maneuvers[-1].duration
        assert begin < end
        return end - begin

    def duration_sec(self) -> float:
        return self.duration_usec() / 10**6

    def starts_at(self) -> int:
        if not self.maneuvers:
            return 0
        return self.maneuvers[0].at

    def ends_at(self) -> int:
        if not self.maneuvers:
            return 0
        return self.maneuvers[-1].at + self.maneuvers[-1].duration

    @staticmethod
    def merge(plans: List["FlightPlan"], squash: bool = True) -> "FlightPlan":
        time_points: List[int] = []
        for plan in plans:
            time_points.extend(plan.time_points())
        time_points = sorted(set(time_points))
        if not time_points:
            return FlightPlan([])

        maneuvers: List[Maneuver] = []
        for i in range(0, len(time_points) - 1):
            begin = time_points[i]
            duration = time_points[i + 1] - begin
            acceleration = Vector(0, 0)
            for plan in plans:
                acceleration += plan.acceleration_at(begin)
            maneuvers.append(Maneuver(at=begin, duration=duration, acc=acceleration))

        return FlightPlan(squash_maneuvers(maneuvers) if squash else maneuvers)

    def apply_to(self, position: Position) -> Position:
        """Predict a new position of the object if this flight plan is
        applied to it."""
        if not self.maneuvers:
            return position
        for maneuver in self.maneuvers:
            position = maneuver.apply_to(position)
        return position

    def partially_apply_to(self, position: Position, duration_usec: int) -> Position:
        assert self.maneuvers and position.timestamp
        assert self.starts_at() <= position.timestamp.usec()
        assert position.timestamp.usec() <= self.ends_at()
        for maneuver in self.maneuvers:
            if position.timestamp.usec() < maneuver.ends_at():
                dt = min(maneuver.ends_at() - position.timestamp.usec(), duration_usec)
                position = maneuver.partially_apply_to(position, dt)
                duration_usec -= dt
                if duration_usec <= 0:
                    break
        return position

    def build_path(self, position: Position, step_ms: int = 1000) -> List[Position]:
        path: List[Position] = []
        while position.timestamp.usec() < self.ends_at():
            position = self.partially_apply_to(position, step_ms * 1000)
            path.append(position)
        return path

def accelerated_from(position: Position, *, acc: float) -> Position:
    '''Return position where object had to start accelerate with
    the specified 'acc' to reach the specified 'position' '''
    if position.velocity.almost_null():
        return copy.deepcopy(position)

    v0 = position.velocity.abs()
    t = v0 / acc
    s = (v0 ** 2) / (2 * acc)
    vector_to_target = position.velocity.set_length(-s, inplace=False)

    now = position.timestamp.usec() if position.timestamp else 0
    timestamp = TimePoint(round(now - t * 10 ** 6), static=True)
    return Position(x = position.x + vector_to_target.x,
                    y = position.y + vector_to_target.y,
                    velocity=Vector(0, 0),
                    timestamp=timestamp)

def decelerating_plan(position: Position, *, acc: float, target_velocity: float = 0) -> FlightPlan:
    """Return a flight plan to stop the ship"""
    if position.velocity.almost_null():
        return FlightPlan([])
    t = (position.velocity.abs() - target_velocity) / acc
    return FlightPlan(
        maneuvers=[
            Maneuver(at=position.timestamp.usec() if position.timestamp else 0,
                     duration=round(t * 10**6),
                     acc=position.velocity.set_length(-acc, inplace=False))
        ]
    )

def one_maneuver_plan(position: Position, target: Position) -> Optional[FlightPlan]:
    # This create a plan with a single maneuver if it is possible
    # But in most cases this plan will take more or much time than plan,
    # created by 'fast_plan()'. So, prefer to use 'fast_plan()' instead.
    if not position.velocity.almost_null():
        if not position.velocity.codirected(position.vector_to(target)):
            return None
        if not target.velocity.almost_null() and \
                not target.velocity.codirected(position.velocity):
            return None

    v0 = position.velocity.abs()
    v1 = target.velocity.abs()
    dv = v1 - v0
    diffSqrV = dv * (v1 + v0)
    s = position.distance_to(target)
    acc = diffSqrV / (2 * s)
    duration = round(1000000 * dv / acc)
    return FlightPlan([Maneuver(
        at=position.timestamp.usec() if position.timestamp else 0,
        duration=duration,
        acc=position.vector_to(target).set_length(acc, inplace=False))])


def fast_plan(position: Position, target: Position, amax: float) -> Optional[FlightPlan]:
    # This create a plan with one or two maneuvers if is is possible
    # In cases where it is possible, it generate the fastest plan to reach the target
    vector_to_target = position.vector_to(target)
    if not position.velocity.almost_null():
        if not position.velocity.codirected(vector_to_target):
            return None
    if not target.velocity.almost_null():
        if not target.velocity.codirected(vector_to_target):
            return None

    v0 = position.velocity.abs()
    v2 = target.velocity.abs()
    s = position.distance_to(target)
    diffSqrV = v2 ** 2 - v0 ** 2
    if vector_to_target.almost_null() and diffSqrV < 0.1:
        # We are already on place:
        return FlightPlan([])

    acc_s = s / 2 + diffSqrV  / (4 * amax)
    if acc_s < 0:
        # thrust doesn't have enough power to decelerate on time
        return None
    if acc_s > s:
        # thrust doesn't have enough power to accelerate on time
        return None

    acc = vector_to_target.set_length(amax)

    # Calculating time for acceleration and deceleration:
    acc_t = (math.sqrt(v0 ** 2 + 2 * amax * acc_s) - v0) / amax
    assert acc_t >= 0

    middle_point = accelerate(start=position, acc=acc, t_sec=acc_t)
    v1 = middle_point.velocity.abs()
    dec_t = (v1 - v2) / amax

    if (acc_t / dec_t < 0.005) or (dec_t / acc_t < 0.005):
        # this should be better done in one maneuver. It will be
        # a bit longer but simplier
        plan = one_maneuver_plan(position, target)
        assert plan
        return plan

    now = position.timestamp.usec() if position.timestamp else 0
    accelerate_maneuver = Maneuver(at=now,
                                   duration=round(acc_t * 10**6),
                                   acc=acc)
    decelerate_maneuver = Maneuver(at=now + round(acc_t * 10**6),
                                   duration=round(dec_t * 10**6),
                                   acc=-acc)
    return FlightPlan(maneuvers=[accelerate_maneuver, decelerate_maneuver])

def stop_at_plan(start: Position, target: Position, amax: float) -> FlightPlan:
    """Return flight plan which can be used to move from the specified 'start'
    position to the specified 'finish'. The 'finish' position must have zero speed.
    Acceleration must not exceed the specified 'amax' value"""
    assert target.velocity.almost_null()

    s = start.distance_to(target)
    now = start.timestamp.usec() if start.timestamp else 0

    simple_plan = fast_plan(start, target, amax)
    if simple_plan:
        return simple_plan

    # Can't decelerate in one maneuver
    assert start.velocity.collinear(start.vector_to(target))
    decelerating = decelerating_plan(start, acc=amax)
    stoped_at = decelerating.apply_to(start)
    stoped_at.velocity.zero()
    moving = fast_plan(stoped_at, target, amax)
    return FlightPlan.merge(plans=[decelerating, moving])

def one_dimension_case(position: Position, target: Position):
    if position.velocity.almost_null():
        if target.velocity.almost_null():
            return True
        else:
            return target.velocity.collinear(position.vector_to(target))
    elif target.velocity.almost_null():
        return position.velocity.collinear(position.vector_to(target))
    else:
        return target.velocity.collinear(position.vector_to(target)) and \
               position.velocity.collinear(position.vector_to(target))

def long_plan(position: Position,
              target: Position,
              amax: float) -> FlightPlan:
    assert one_dimension_case(position, target)

    now = position.timestamp.usec() if position.timestamp else 0

    # Plan A: stop the ship and accelerate to target
    plan_a_decelerate = decelerating_plan(position, acc=amax)
    stop_position = plan_a_decelerate.apply_to(position)
    stop_position.velocity.zero()
    plan_a_accelerate = fast_plan(stop_position, target, amax=amax)
    if plan_a_accelerate:
        return FlightPlan.merge([plan_a_decelerate, plan_a_accelerate])

    # Plan B: calculate a point where we should stop to start
    # accelerate, reach that point and do acceleration
    start_accelerate_position = accelerated_from(target, acc=0.9999*amax)
    plan_b_decelerate = fast_plan(position, start_accelerate_position, amax=amax)
    if (plan_b_decelerate):
        start_accelerate_position.timestamp.update(now + plan_b_decelerate.duration_usec())
        plan_b_accelerate = one_maneuver_plan(start_accelerate_position, target)
        assert plan_b_accelerate and plan_b_accelerate.max_acceleration() < amax
        return FlightPlan.merge([plan_b_decelerate, plan_b_accelerate])

    # Plan C: stop the ship, move to accelerate position, do acceleration
    plan_c_decelerate = plan_a_decelerate  # Yep, they are the same
    plan_c_middle = fast_plan(stop_position, start_accelerate_position, amax=amax)
    start_accelerate_position.timestamp.update(
        stop_position.timestamp.usec() + plan_c_middle.duration_usec())
    plan_c_accelerate = one_maneuver_plan(start_accelerate_position, target)
    assert plan_c_accelerate and plan_c_accelerate.max_acceleration() < amax
    return FlightPlan.merge([plan_c_decelerate, plan_c_middle, plan_c_accelerate])

def prepare_flight_plan_1D(position: Position,
                           target: Position,
                           amax: float) -> FlightPlan:
    # As far as we are in 1D the following assertions should be true:
    assert one_dimension_case(position, target)
    maneuvers: List[Maneuver] = []

    # Try to build one manuever plan first
    simple_plan = fast_plan(position, target, amax)
    if simple_plan:
        # Gods are smiling for us today: it's a simple case
        return simple_plan

    # We are not that lucky, we have to do a series of manuevers
    return long_plan(position, target, amax)

class Range:
    def __init__(self, left: float, right: float):
        self.left = left
        self.right = right

    def middle(self) -> float:
        return (self.right + self.left) / 2

    def length(self) -> float:
        return self.right - self.left

    def shift_left(self):
        self.left = self.middle()

    def extend_left(self):
        self.left = 2 * self.left - self.right

    def extend_right(self):
        self.right = 2 * self.right - self.left

    def shift_right(self):
        self.right = self.middle()

    def closest_to_zero(self,
                        predicate: Callable[[float], float],
                        good_enough: Optional[float] = None,
                        max_cycles: int = 32) -> Tuple[float, float]:
        """Find alfa where f has a value closest to 0 (could be negative).
        Return alfa and corresponding predicate's value
        Note: f must be monotonically increasing
        """
        best = (self.right, abs(predicate(self.right)))
        if good_enough and best[1] < good_enough:
            # We can't be that lucky!
            return best

        for i in range(max_cycles):
            alfa = self.middle()
            deviation = predicate(self.middle())
            if good_enough and abs(deviation) < good_enough:
                return (alfa, deviation)
            if abs(deviation) < best[1]:
                best = (alfa, deviation)
            if deviation < 0:
                self.shift_left()
            else:
                self.shift_right()

        return best

    def smallest_positive(self,
                          predicate: Callable[[float], float],
                          good_enough: Optional[float] = None,
                          max_cycles: int = 32) -> Tuple[Optional[float], float]:
        """Find alfa where f has smallest positive value. Return alfa and
        corresponding predicate's value
        Note: f must be monotonically increasing
        """
        best = (self.right, predicate(self.right))
        if best[1] < 0:
            # The whole f is negative within the specified range
            return None, best[1]
        if good_enough and best[1] < good_enough:
            return best

        for i in range(0, max_cycles):
            alfa = self.middle()
            deviation = predicate(alfa)
            if deviation < 0:
                self.shift_left()
            elif good_enough and deviation < good_enough:
                return alfa, deviation
            else:
                best = (alfa, deviation)
                self.shift_right()
        return best

    def extend_bounds(self, f: Callable[[float], float]):
        """Extend bounds until f is negative for left bound and positive for
        right bound"""
        while f(self.left) > 0:
            self.extend_left()
        while f(self.right) < 0:
            self.extend_right()
        return self

    def iterate(self, step: float):
        i = self.left
        while i < self.right:
            yield i
            i += step

def _prepare_flight_plan(position: Position,
                         target: Position,
                         plan_x_builder: Callable[[Position, Position, float], FlightPlan],
                         plan_y_builder: Callable[[Position, Position, float], FlightPlan],
                         amax: float) -> Optional[FlightPlan]:
    target_y = Position(x=target.x, y=target.y, velocity=Vector(0, 0))
    x_position, y_position = position.decompose(target)

    def predicate(alfa: float) -> int:
        plan_x = plan_x_builder(x_position, target, amax * math.cos(alfa))
        plan_y = plan_y_builder(y_position, target_y, amax * math.sin(alfa))
        if not plan_x:
            return 0 - plan_y.duration_usec()
        return plan_x.duration_usec() - plan_y.duration_usec()

    range = Range(0.01 * math.pi / 2, 0.99 * math.pi / 2)
    alfa, _ = range.smallest_positive(predicate, good_enough=10**4)
    if alfa:
        plan_x = plan_x_builder(x_position, target, amax * math.cos(alfa))
        plan_y = plan_y_builder(y_position, target_y, amax * math.sin(alfa))
        # No need to squash since plan_x and plan_y are already squashed
        return FlightPlan.merge([plan_x, plan_y], squash=False)
    return None

def prepare_fast_flight_plan(position: Position, target: Position, amax: float) \
        -> Optional[FlightPlan]:
    return _prepare_flight_plan(
        position=position,
        target=target,
        plan_x_builder=fast_plan,
        plan_y_builder=stop_at_plan,
        amax=amax)

def prepare_long_flight_plan(position: Position, target: Position, amax: float) \
        -> Optional[FlightPlan]:
    return _prepare_flight_plan(
        position=position,
        target=target,
        plan_x_builder=long_plan,
        plan_y_builder=stop_at_plan,
        amax=amax)

def prepare_flight_plan(position: Position, target: Position, amax: float) -> Optional[FlightPlan]:
    if target.timestamp is None:
        if one_dimension_case(position, target):
            return prepare_flight_plan_1D(position, target, amax)

        plan = prepare_fast_flight_plan(position, target, amax)
        if not plan:
            plan = prepare_long_flight_plan(position, target, amax)
        return plan

    else:
        # Should be on target's position at the specified time.
        # We can only manipulate with 'amax'
        dt = target.timestamp.usec() - position.timestamp.usec()
        if dt < 1000:
            return None

        def predicate_fast(amax: float):
            plan = prepare_flight_plan(position, target.no_timestamp(), amax)
            duration = plan.duration_usec()
            return dt - duration

        def predicate_long(amax: float):
            plan = prepare_long_flight_plan(position, target.no_timestamp(), amax)
            duration = plan.duration_usec()
            return dt - duration

        good_enough = 10000  # 10 ms is good enough
        for predicate in [predicate_fast, predicate_long]:
            acc, dt = Range(0.01, amax).closest_to_zero(predicate, good_enough)
            if acc and abs(dt) < good_enough:
                return prepare_flight_plan(position, target.no_timestamp(), acc)
        return None


def time_to_reach(position: Position, target: Position) -> float:
    assert position.velocity.codirected(position.vector_to(target))
    s = max(target.x - position.x, target.y - position.y)
    v = max(position.velocity.x, position.velocity.y)
    return s / v


def approach_to_plan(position: Position, target: Position, amax: float,
                     precise_level: int = 8) -> FlightPlan:

    def predicate(t: int) -> bool:
        at = target.timestamp.usec() + t
        predicted_target = target.predict(at)
        plan = prepare_flight_plan(position, predicted_target, amax)
        return plan is not None

    # try to estimate initial right bound
    estimation_plan = prepare_flight_plan(position, target.no_timestamp(), amax)

    # This predicate will return false at t = 0. All we need to do is to
    # find minimal t, where predicate returns True
    t_range = Range(0, estimation_plan.duration_usec())
    defined = predicate(t_range.right)
    while not defined:
        t_range.left = t_range.right
        t_range.right *= 2
        defined = predicate(t_range.right)

    # Function is defined on right bound and not defined on left
    # bound. Now we should find minimal t where it is defined
    for attempts in range(0, precise_level):
        defined = predicate(t_range.middle())
        if defined:
            t_range.shift_right()
        else:
            t_range.shift_left()

    flight_time = t_range.right  # predicate is defined on right bound
    randevu_t = target.timestamp.usec() + flight_time
    randevu_position = target.predict(randevu_t)
    return prepare_flight_plan(position, randevu_position, amax)


async def follow_flight_plan(
        ship: Ship,
        engine: Engine,
        plan: FlightPlan,
        system_clock: rpc.SystemClockI) -> bool:
    token = random.randint(0, 100)
    now = await system_clock.time()
    for maneuver in plan.maneuvers:
        now = await system_clock.wait_until(time=maneuver.at - 25000)

        ship_state = await ship.get_state()
        if ship_state is None:
            return False

        thrust = ship_state.weight * maneuver.acc.abs()
        if not await engine.set_thrust(
                thrust=maneuver.acc.set_length(thrust, inplace=False),
                at=maneuver.at,
                duration_ms=round(maneuver.duration / 1000)):
            return False


    # Waiting last maneuver to finish
    if plan.maneuvers:
        await system_clock.wait_until(time=plan.ends_at(), timeout=0)
    return True
