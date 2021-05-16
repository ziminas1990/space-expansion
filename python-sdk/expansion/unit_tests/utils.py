import random as rnd
import expansion.types as types
import math

def random_vector(length: float) -> types.Vector:
    alfa = rnd.random() * math.tau
    return types.Vector(x= length * math.cos(alfa), y = length * math.sin(alfa))

def random_position(x: float, y: float, deviation: float, speed_max: float) -> types.Position:
    alfa = rnd.random() * math.tau
    r = rnd.random() * deviation
    return types.Position(x = r * math.cos(alfa),
                          y = r * math.sin(alfa),
                          velocity=random_vector(speed_max))

def vectors_equal(one: types.Vector, other: types.Vector) -> bool:
    diff = one - other
    return diff.abs() < 0.000001
