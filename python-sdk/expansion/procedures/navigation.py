from typing import Callable, Awaitable, NamedTuple, List, Tuple, Optional, Iterator
import math
import asyncio

from expansion.types import Position, Vector
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
            assert position.timestamp <= self.at
            position = position.predict(self.at)

        dt = self.duration / 1000000
        return Position(
            x = position.x + position.velocity.x * dt + self.acc.x * dt * dt / 2,
            y = position.y + position.velocity.y * dt + self.acc.y * dt * dt / 2,
            velocity=Vector(
                x = position.velocity.x + self.acc.x * dt,
                y = position.velocity.y + self.acc.y * dt),
            timestamp = self.at + self.duration
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

    def acceleration_at(self, at_us: int) -> Vector:
        for maneuver in self.maneuvers:
            if at_us < maneuver.at:
                return Vector(0, 0)
            if maneuver.at <= at < maneuver.at + maneuver.duration:
                return maneuver.acc
        return Vector(0, 0)

    @staticmethod
    def merge(self, plans: List["FlightPlan"]) -> "FlightPlan":
        time_points: List[int] = []
        for plan in plans:
            time_points.extend(plan.time_points())
        time_points.sort()
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


def flight_through_plan(position: Position, *, x: float, y: float, amax: float) -> FlightPlan:
    """Return a flight plan to reach the speicfied point with any speed"""
    vector_to_target = position.vector_to((x, y))
    cosa = position.velocity.cosa(vector_to_target)
    s = position.distance_to((x, y))
    if (abs(cosa) > 0.999):
        # 1D case
        v0 = position.velocity.abs() * cosa  # it can be negative
        D = v0 * v0 + 2 * amax * s  # discriminant can't be negative btw
        t = (-v0 + math.sqrt(D)) / amax
        return FlightPlan(maneuvers=[
            Maneuver(at=0, duration=t * 1000000, acc=vector_to_target.set_length(amax))
        ])
    else:
        # 2D case
        assert False, "Not supported"


def accelerate(start: Position, acc: Vector, t_sec: float) -> Position:
    dv = acc * t_sec
    ds = start.velocity * t_sec + (dv * t_sec) / 2
    timestamp = start.timestamp + t_sec * 10 ** 6 if start.timestamp is not None else None
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
    timestamp = position.timestamp - (t * 1000000) if position.timestamp is not None else None
    return Position(x = position.x + vector_to_target.x,
                    y = position.y + vector_to_target.y,
                    velocity=Vector(0, 0),
                    timestamp=timestamp)

def decelerated_at(position: Position, *, acc: float) -> Position:
    t = position.velocity.abs() / acc
    s = t * position.velocity.abs() / 2
    vector_to_target = position.velocity.set_length(s, inplace=False)
    timestamp = position.timestamp - (t * 1000000) if position.timestamp is not None else None
    return Position(x=position.x + vector_to_target.x,
                    y=position.y + vector_to_target.y,
                    velocity=Vector(0, 0),
                    timestamp=timestamp)


def prepare_flight_plan_1D(position: Position, target: Position) -> FlightPlan:
    # As far as we are in 1D the following assertions should be true:
    assert 0.9999 < abs(position.velocity.cosa(position.vector_to(target))) < 1.0001
    assert 0.9999 < abs(position.velocity.cosa(target.velocity)) < 1.0001

    # more specific assertions that prevent cases which are not supported yet
    assert 0.9999 < position.velocity.cosa(target.velocity) < 1.0001

    maneuvers: List[Maneuver] = []

    dv = target.velocity.abs() - position.velocity.abs()
    diffSqrV = dv * (target.velocity.abs() + position.velocity.abs())
    s = position.distance_to(target)
    acc = diffSqrV / (2 * s)
    duration = round(1000000 * dv / acc)
    maneuvers.append(Maneuver(at=position.timestamp if position.timestamp else 0,
                              duration=duration,
                              acc=position.velocity.set_length(acc, inplace=False)))
    return FlightPlan(maneuvers)


def prepare_flight_plan_2D(position: Position, target: Position) -> FlightPlan:
    # more specific assertions that prevent cases which are not supported yet
    assert 0 < position.vector_to(target).cosa(target.velocity)

    longitudinal, lateral = position.decompose(target)
    longitudinal_plan = prepare_flight_plan_1D(longitudinal, target)
    lateral_target = Position(x=target.x, y=target.y, velocity=Vector(0, 0))
    lateral_plan = prepare_flight_plan_1D(lateral, target)
    return FlightPlan.merge([longitudinal_plan, lateral_plan])


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
        thrust = dv
        thrust.set_length(engine_max_thrust)

        # Heuristic: no sudden movements, please
        if burn_time < 0.5:
            thrust.mult_self(burn_time / 0.5)
            burn_time = 0.5

        if not await engine.set_thrust(thrust=thrust, duration_ms=int(round(burn_time * 1000))):
            return False

        sleep_time = burn_time / 2
        if sleep_time > 2:
            sleep_time = 2
        now_us = await system_clock.wait_for(int(sleep_time * 1000000), 2 * sleep_time)
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
