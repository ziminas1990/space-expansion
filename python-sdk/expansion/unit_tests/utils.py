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
                          velocity=random_vector(speed_max),
                          timestamp=types.TimePoint(timestamp, static=True))

def vectors_equal(one: types.Vector, other: types.Vector, delta: float = 0.000001) -> bool:
    diff = one - other
    return diff.abs() < delta

def positions_equal(one: types.Position, other: types.Position,
                    ds: float = 0.01, dv: float = 0.01) -> bool:
    return one.distance_to(other) < ds \
           and vectors_equal(one.velocity, other.velocity, dv)
