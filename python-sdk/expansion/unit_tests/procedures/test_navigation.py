import unittest
import random as rnd
import expansion.types as types
from expansion.unit_tests.utils import random_position, vectors_equal, positions_equal
from expansion.procedures.navigation import (
    prepare_flight_plan_1D,
    flight_through_plan,
    accelerated_from,
    decelerated_at,
    accelerate
)
import math


class TestCase(unittest.TestCase):

    def create_target_on_line(self, position: types.Position, distance: float,
                              maxspeed: float = 10000):
        vector_to_target = position.velocity.set_length(distance, inplace=False)
        return types.Position(
            x=position.x + vector_to_target.x,
            y=position.y + vector_to_target.y,
            velocity=position.velocity.set_length(rnd.random() * maxspeed, inplace=False)
        )

    def create_target(self, position: types.Position, distance: float):
        vector_to_target = position.velocity.set_length(distance, inplace=False)
        return types.Position(
            x=position.x + vector_to_target.x,
            y=position.y + vector_to_target.y,
            velocity=position.velocity.set_length(rnd.random() * 10000, inplace=False)
        )

    def test_accelerated_from(self):
        rnd.seed(43653)
        for i in range(10000):
            case_seed = rnd.randint(1, 1000000)
            rnd.seed(case_seed)
            position = random_position(0, 0, 1000, 1000, timestamp=10**8)
            amax = rnd.random() * 1000
            # Calculating start position
            start_position = accelerated_from(position, acc=amax)
            # Verify
            dt = (position.timestamp - start_position.timestamp) / 1000000
            arrive = accelerate(start_position,
                                acc=position.velocity.set_length(amax, inplace=False),
                                t_sec=dt)
            positions_equal(arrive, position)

    def test_decelerated_at(self):
        rnd.seed(43653)
        for i in range(10000):
            case_seed = rnd.randint(1, 1000000)
            rnd.seed(case_seed)
            position = random_position(0, 0, 1000, 1000, timestamp=0)
            amax = rnd.random() * 1000
            # Calculating stop position
            stop_position = decelerated_at(position, acc=amax)
            # Verify
            dt = (stop_position.timestamp - position.timestamp) / 1000000
            arrive = accelerate(position,
                                acc=position.velocity.set_length(-amax, inplace=False),
                                t_sec=dt)
            positions_equal(arrive, stop_position)


    def test_1D_flight_through(self):
        rnd.seed(43653)
        for i in range(10000):
            case_seed = rnd.randint(1, 1000000)
            rnd.seed(case_seed)
            position = random_position(0, 0, 1000, 1000)
            target = self.create_target_on_line(
                position=position,
                distance=0.1 + rnd.random() * 100000,
                maxspeed=0)
            amax = rnd.random() * 1000
            # Build flight plan
            plan = flight_through_plan(position, x=target.x, y=target.y, amax=amax)
            # Predict flight plan result
            arrive = plan.apply_to(position)
            self.assertLess(arrive.distance_to(target), 0.01)

    def test_prepare_fligh_plan_1D(self):
        rnd.seed(74836)
        for i in range(10000):
            case_seed = rnd.randint(1, 1000000)
            rnd.seed(case_seed)
            # Create position ant target
            position = random_position(0, 0, 1000, 1000)
            target = self.create_target_on_line(
                position=position,
                distance=0.1 + rnd.random() * 100000)
            # Build flight plan
            plan = prepare_flight_plan_1D(position, target)
            # Predict flight plan result
            arrive = plan.apply_to(position)
            self.assertTrue(positions_equal(
                one=target,
                other=arrive,
                ds=position.distance_to(target) * 0.0001,
                dv=(target.velocity - position.velocity).abs() * 0.0001))

    def _test_prepare_fligh_plan_2D(self):
        rnd.seed(74836)
        for i in range(10000):
            case_seed = rnd.randint(1, 1000000)
            rnd.seed(case_seed)
            # Create position ant target
            position = random_position(0, 0, 100, 100)
            target = self.create_target(
                position=position,
                distance=0.1 + rnd.random() * 100000)
            # Build flight plan
            plan = prepare_flight_plan_1D(position, target)
            # Predict flight plan result
            arrive = plan.apply_to(position)
            self.assertTrue(positions_equal(target, arrive, delta=0.1))
