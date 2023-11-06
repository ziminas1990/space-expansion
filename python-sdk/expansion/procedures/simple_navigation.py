from typing import NamedTuple, List

from expansion.types import Position, Vector, TimePoint
from navigation import Maneuver


def build_intercept_plan(position: Position, target: Position): List[Maneuver]:
    if not position.velocity.almost_null():

