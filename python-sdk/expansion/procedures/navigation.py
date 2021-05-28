from typing import Callable, Awaitable, NamedTuple, List, Tuple, Optional, Iterator
import math
import asyncio

from expansion.types import Position, Vector, TimePoint
from expansion.modules.ship import Ship
from expansion.modules.engine import Engine, EngineSpec
import expansion.interfaces.rpc as rpc


class Maneuver(NamedTuple):
    at: int        # When the specified acceleration should be set
    duration: int  # How long acceleration should last
    acc: Vector    # An acceleration that should be set

    def __cmp__(self, other: "Maneuver"):
        return self.at - other.at

    def apply_to(self, position: Position) -> Position:
        """Predict a new position of the object if this maneuver is applied to it."""
        if position.timestamp:
            assert position.timestamp.usec() <= self.at
            position = position.predict(self.at)

        dt = self.duration / 1000000
        return Position(
            x = position.x + dt * (position.velocity.x + self.acc.x * dt / 2),
            y = position.y + dt * (position.velocity.y + self.acc.y * dt / 2),
            velocity=Vector(
                x = position.velocity.x + self.acc.x * dt,
                y = position.velocity.y + self.acc.y * dt),
            timestamp = TimePoint(self.at + self.duration, static=True)
        )

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

    @staticmethod
    def merge(plans: List["FlightPlan"]) -> "FlightPlan":
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

        return FlightPlan(maneuvers)

    def apply_to(self, position: Position) -> Position:
        """Predict a new position of the object if this flight plan is
        applied to it."""
        if not self.maneuvers:
            return position
        for maneuver in self.maneuvers:
            position = maneuver.apply_to(position)
        return position

def accelerate(start: Position, acc: Vector, t_sec: float) -> Position:
    """Return a result of acceleration with the specified 'acc' from the
    specified 'start' position during the specified 't_sec' seconds"""
    dv = acc * t_sec
    ds = start.velocity * t_sec + (dv * t_sec) / 2
    timestamp = None
    if start.timestamp is not None:
        timestamp = TimePoint(start.timestamp.usec() + (t_sec * 10 ** 6), static=True)
    return Position(x = start.x + ds.x,
                    y = start.y + ds.y,
                    velocity = start.velocity + dv,
                    timestamp = timestamp)

def accelerated_from(position: Position, *, acc: float) -> Position:
    '''Return position where object had to start accelerate with
    the specified 'acc' to reach the specified 'position' '''
    t = position.velocity.abs() / acc
    s = acc * t * t / 2
    vector_to_target = position.velocity.set_length(-s, inplace=False)
    timestamp = None
    if position.timestamp is not None:
        timestamp = TimePoint(position.timestamp.usec() - (t * 10 ** 6), static=True)
    return Position(x = position.x + vector_to_target.x,
                    y = position.y + vector_to_target.y,
                    velocity=Vector(0, 0),
                    timestamp=timestamp)

def decelerating_plan(position: Position, *, acc: float) -> FlightPlan:
    """Return a flight plan to stop the ship"""
    t = position.velocity.abs() / acc
    return FlightPlan(
        maneuvers=[
            Maneuver(at=0,
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

    dv = target.velocity.abs() - position.velocity.abs()
    diffSqrV = dv * (target.velocity.abs() + position.velocity.abs())
    s = position.distance_to(target)
    acc = diffSqrV / (2 * s)
    duration = round(1000000 * dv / acc)
    return FlightPlan([Maneuver(
        at=position.timestamp.usec() if position.timestamp else 0,
        duration=duration,
        acc=position.vector_to(target).set_length(acc, inplace=False))])


def stop_at_plan(start: Position, finish: Position, amax: float) -> FlightPlan:
    """Return flight plan which can be used to move from the specified 'start'
    position to the specified 'finish'. The 'finish' position must have zero speed.
    Acceleration must not exceed the specified 'amax' value"""
    assert finish.velocity.abs() < 0.001

    s = start.distance_to(finish)
    now = start.timestamp.usec() if start.timestamp else 0

    if start.velocity.abs() < 0.001:
        t = math.sqrt(s/amax)
        return FlightPlan(maneuvers=[
            Maneuver(at=now,
                     duration=t * 10**6,
                     acc=start.vector_to(finish).set_length(amax)),
            Maneuver(at=now + t * 10**6,
                     duration=t * 10 ** 6,
                     acc=start.vector_to(finish).set_length(-amax)),
        ])

    cosa = start.velocity.cosa(start.vector_to(finish))
    if 0.999 < abs(cosa):
        # 1D case:
        # Try to build one maneuver plan first
        simple_plan = one_maneuver_plan(start, finish)
        if simple_plan and simple_plan.max_acceleration() < amax:
            return simple_plan
        # Can't decelerate in one maneuver
        decelerating = decelerating_plan(start, acc=amax)
        stoped_at = decelerating.apply_to(start)
        stoped_at.velocity.set_length(0)
        moving = stop_at_plan(stoped_at, finish, amax)
        return FlightPlan.merge(plans=[decelerating, moving])
    else:
        assert False  # Not supported


def prepare_flight_plan_1D(position: Position,
                           target: Position,
                           amax: float) -> FlightPlan:
    # As far as we are in 1D the following assertions should be true:
    if position.velocity.abs() > 0.001:
        assert 0.9999 < abs(position.vector_to(target).cosa(position.velocity))
    if target.velocity.abs() > 0.001:
        assert 0.9999 < abs(position.vector_to(target).cosa(target.velocity))
    maneuvers: List[Maneuver] = []

    # Try to build one manuever plan first
    simple_plan = one_maneuver_plan(position, target)
    if simple_plan and simple_plan.max_acceleration() < amax:
        # Gods are smiling for us today: it's a simple case
        return simple_plan

    if target.velocity.abs() < 0.01:
        return stop_at_plan(position, target, amax)

    # We are not that lucky, we have to do a series of manuevers
    # Let's find 'middle_point'. It should be a point where we should start to
    # accelerate with amax to reach the target
    middle_point = accelerated_from(target, acc=amax * 0.995)
    # Building a plan to reach middle_point
    decelerate = stop_at_plan(position, middle_point, amax)
    # Recalculating middle_point to calculate it's timestamp
    middle_point = decelerate.apply_to(position)
    middle_point.velocity.set_length(0)
    # Calculatin a plan to accelerate from middle point to target
    # This time it should be a one maneuver plan
    accelerate = one_maneuver_plan(middle_point, target=target)
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
    return FlightPlan.merge([plan_x, plan_y])


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
