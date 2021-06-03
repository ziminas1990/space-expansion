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
        if (position.timestamp.usec() + duration_usec > self.ends_at()):
            print("fuck")
        assert position.timestamp.usec() + duration_usec <= self.ends_at()
        for maneuver in self.maneuvers:
            if position.timestamp.usec() < maneuver.ends_at():
                dt = min(maneuver.ends_at() - position.timestamp.usec(), duration_usec)
                position = maneuver.partially_apply_to(position, dt)
                duration_usec -= dt
                if duration_usec <= 0:
                    break
        return position


def accelerated_from(position: Position, *, acc: float) -> Position:
    '''Return position where object had to start accelerate with
    the specified 'acc' to reach the specified 'position' '''
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

def decelerating_plan(position: Position, *, acc: float) -> FlightPlan:
    """Return a flight plan to stop the ship"""
    t = position.velocity.abs() / acc
    return FlightPlan(
        maneuvers=[
            Maneuver(at=position.timestamp.usec() if position.timestamp else 0,
                     duration=round(t * 10**6),
                     acc=position.velocity.set_length(-acc, inplace=False))
        ]
    )

def one_maneuver_plan(position: Position, target: Position) -> Optional[FlightPlan]:
    # This algorytm can be used only if:
    # 1 initial position has no speed
    # 2 if initial position has a speed than:
    #   2.1 speed is directed to the target
    #   2.2 initial speed and target speed have the same direction
    # This algorytm can't be used in other cases
    if position.velocity.abs() > 0.002:
        if position.vector_to(target).cosa(position.velocity) < 0.999:
            return None
        if target.velocity.abs() > 0.002 and \
                position.velocity.cosa(target.velocity) < 0.999:
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


def two_maneuvers_plan(position: Position, target: Position, amax: float):
    # This algorytm can't be used in other cases
    vector_to_target = position.vector_to(target)
    if position.velocity.abs() > 0.002:
        if not position.velocity.codirected(vector_to_target):
            return None
    if target.velocity.abs() > 0.002:
        if not target.velocity.codirected(vector_to_target):
            return None

    v0 = position.velocity.abs()
    v2 = target.velocity.abs()
    s = position.distance_to(target)
    diffSqrV = v2 ** 2 - v0 ** 2
    acc_s = s / 2 + diffSqrV  / (4 * amax)
    if acc_s < 0:
        # thrust doesn't have enough power to decelerate on time
        return None
    if acc_s > s:
        # thrust doesn't have enough power to accelerate on time
        return None

    acc = vector_to_target.set_length(amax)

    acc_t = (math.sqrt(v0 ** 2 + 2 * amax * acc_s) - v0) / amax
    assert acc_t >= 0

    middle_point = accelerate(start=position, acc=acc, t_sec=acc_t)
    v1 = middle_point.velocity.abs()
    dec_t = (v1 - v2) / amax

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
    assert target.velocity.abs() < 0.001

    s = start.distance_to(target)
    now = start.timestamp.usec() if start.timestamp else 0

    simple_plan = two_maneuvers_plan(start, target, amax)
    if simple_plan:
        return simple_plan

    # Can't decelerate in one maneuver
    assert start.velocity.collinear(start.vector_to(target))
    decelerating = decelerating_plan(start, acc=amax)
    stoped_at = decelerating.apply_to(start)
    stoped_at.velocity.zero()
    moving = two_maneuvers_plan(stoped_at, target, amax)
    if not decelerating or not moving:
        print("aaa")
    return FlightPlan.merge(plans=[decelerating, moving])

def one_dimension_case(position: Position, target: Position):
    if position.velocity.abs() < 0.002:
        if target.velocity.abs() < 0.002:
            return True
        else:
            return target.velocity.collinear(position.vector_to(target))
    elif target.velocity.abs() < 0.002:
        return position.velocity.collinear(position.vector_to(target))
    else:
        return target.velocity.collinear(position.vector_to(target)) and \
               position.velocity.collinear(position.vector_to(target))

def prepare_flight_plan_1D(position: Position,
                           target: Position,
                           amax: float) -> FlightPlan:
    # As far as we are in 1D the following assertions should be true:
    assert one_dimension_case(position, target)
    maneuvers: List[Maneuver] = []

    # Try to build one manuever plan first
    simple_plan = two_maneuvers_plan(position, target, amax)
    if simple_plan:
        # Gods are smiling for us today: it's a simple case
        return simple_plan

    if target.velocity.abs() < 0.01:
        return stop_at_plan(position, target, amax)

    now = position.timestamp.usec() if position.timestamp else 0

    # We are not that lucky, we have to do a series of manuevers
    # Let's find 'middle_point'. It should be a point where we should start to
    # accelerate with amax to reach the target
    middle_point = accelerated_from(target, acc=amax * 0.99)
    # Building a plan to reach middle_point
    decelerate = stop_at_plan(position, middle_point, amax)
    # Recalculating middle_point to calculate it's timestamp
    middle_point.timestamp.update(now + decelerate.duration_usec())
    # Calculatin a plan to accelerate from middle point to target
    # This time it should be a one maneuver plan
    accelerate = one_maneuver_plan(middle_point, target=target)
    if not (accelerate and accelerate.max_acceleration() <= amax):
        print(accelerate.max_acceleration(), amax)
    assert accelerate and accelerate.max_acceleration() <= amax
    # Finally, merging (actually, concatanating) two plans:
    return FlightPlan.merge([decelerate, accelerate])


class Range:
    def __init__(self, left: float, right: float):
        self.left = left
        self.right = right

    def middle(self) -> float:
        return (self.right + self.left) / 2

    def shift_left(self):
        self.left = self.middle()

    def shift_right(self):
        self.right = self.middle()

    def calculate_zero(self, f: Callable[[float], float], cycles: int = 32) -> "Range":
        bounds = Range(self.left, self.right)
        for i in range(cycles):
            if f(bounds.middle()) > 0:
                bounds.shift_right()
            else:
                bounds.shift_left()
        return bounds


def prepare_flight_plan(position: Position, target: Position, amax: float) -> FlightPlan:
    if one_dimension_case(position, target):
        return prepare_flight_plan_1D(position, target, amax)

    target_y = Position(x=target.x, y=target.y, velocity=Vector(0, 0))
    x_position, y_position = position.decompose(target)

    def predicate(alfa: float):
        plan_x = prepare_flight_plan_1D(x_position, target, amax * math.cos(alfa))
        plan_y = prepare_flight_plan_1D(y_position, target_y, amax * math.sin(alfa))
        return plan_x.duration_usec() - plan_y.duration_usec()

    # Use right bound instead of middle because we need strongly positive
    # predicate's value!
    alfa = Range(0, math.pi / 2).calculate_zero(predicate).right
    plan_x = prepare_flight_plan_1D(x_position, target, amax * math.cos(alfa))
    plan_y = prepare_flight_plan_1D(y_position, target_y, amax * math.sin(alfa))
    # No need to squash since plan_x and plan_y are already squashed
    return FlightPlan.merge([plan_x, plan_y], squash=False)


async def move_to(ship: Ship,
                  engine: Engine,
                  get_target: Callable[[], Awaitable[Position]],
                  system_clock: rpc.SystemClockI,
                  max_distance_error: float = 5,
                  max_velocity_error: float = 0.5,
                  max_thrust_k_limit: float = 1) -> bool:
    """Move the specified 'ship' to the target returned by the specified
    'get_target' callback using the specified 'engine'. Ths specified 'system_clock'
    will be used to track the server's time. Procedure will complete when a
    distance between ship and 'position' is not more than the specified
    'max_distance_error' meters AND the ship's speed is not more than the specified
    'max_velocity_error'.
    """
    ship_state: rpc.ShipState = await ship.get_state()
    target = await get_target()
    engine_spec: EngineSpec = await engine.get_specification()
    if not ship_state or not target or not engine_spec:
        return False
    engine_max_thrust = engine_spec.max_thrust * max_thrust_k_limit

    distance = ship_state.position.distance_to(target)
    relative_v = ship_state.position.velocity - target.velocity

    while distance > max_distance_error or relative_v.abs() > max_velocity_error:
        max_acceleration = engine_max_thrust / ship_state.weight

        # Calculating the best relative velocity vector for this moment
        # Best velocity has such value, that if ship starts slow down right now, it will stop
        # at the required position
        best_velocity = ship_state.position.vector_to(target)
        best_velocity.set_length(math.sqrt(2 * distance * max_acceleration))

        # dv - how velocity should be changed to become perfect
        dv = best_velocity - relative_v
        burn_time = dv.abs() * ship_state.weight / engine_max_thrust

        # Calculating thrust
        thrust = dv.set_length(engine_max_thrust, inplace=False)

        # Heuristic: no sudden movements, please
        if burn_time < 0.5:
            thrust.mult_self(burn_time / 0.5)
            burn_time = 0.5

        if not await engine.set_thrust(thrust=thrust, duration_ms=int(round(burn_time * 1000))):
            return False

        sleep_time = burn_time / 2
        if sleep_time > 2:
            sleep_time = 2
        now_us = await system_clock.wait_for(int(sleep_time * 10**6), 2 * sleep_time)
        if now_us is None:
            # Something went wrong on server (may be lags?)
            return False

        # Getting actual data:
        ship_state, target = \
            await asyncio.gather(ship.get_state(cache_expiring_ms=0),
                                 get_target())
        if not ship_state or not target:
            return False
        distance = ship_state.position.distance_to(target)
        relative_v = ship_state.position.velocity - target.velocity
    return True
