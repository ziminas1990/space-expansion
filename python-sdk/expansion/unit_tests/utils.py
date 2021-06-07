from typing import Tuple
import random
import random as rnd
import expansion.types as types
import math

def random_vector(length: float) -> types.Vector:
    alfa = rnd.random() * math.tau
    return types.Vector(x= length * math.cos(alfa), y = length * math.sin(alfa))

def random_position(x: float, y: float, deviation: float, speed_max: float,
                    timestamp: int = 0) -> types.Position:
    alfa = rnd.random() * math.tau
    r = rnd.random() * deviation
    return types.Position(x = r * math.cos(alfa),
                          y = r * math.sin(alfa),
                          velocity=random_vector(rnd.random() * speed_max),
                          timestamp=types.TimePoint(timestamp, static=True))

def random_positions_1D(x: float, y: float, radius: float,
                        max_start_speed: float, max_stop_speed: float,
                        start_timestamp: int = 0) -> Tuple[types.Position, types.Position]:
    start = random_position(x, y, radius, 0)
    stop = random_position(x, y, radius, 0)
    start_velocity = start.vector_to(stop).set_length(max_start_speed * rnd.random())
    stop_velocity = start.vector_to(stop).set_length(max_stop_speed * rnd.random())
    return types.Position(start.x, start.y, start_velocity,
                          types.TimePoint(start_timestamp, static=True)), \
           types.Position(stop.x, stop.y, stop_velocity, None)


def vectors_equal(one: types.Vector, other: types.Vector, delta: float = 0.000001) -> bool:
    if delta == 0.0:
        delta += 0.00001
    diff = one - other
    return diff.abs() < delta

def positions_equal(one: types.Position, other: types.Position,
                    ds: float = 0.01, dv: float = 0.01) -> bool:
    return one.distance_to(other) < ds \
           and vectors_equal(one.velocity, other.velocity, dv)
