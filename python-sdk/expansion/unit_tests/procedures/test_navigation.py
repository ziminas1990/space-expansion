import math
import random
import unittest
from typing import List

from expansion.procedures.navigation import (
    FlightPlan,
    approach_to_plan,
    prepare_flight_plan_in_delta_v,
    prepare_flight_plan_in_time,
)
from expansion.types import Position, TimePoint, Vector


CASES = 1_000
POSITION_DELTA = 5
VELOCITY_DELTA = 1
INNER_WINDOWS = 3


def random_kinematic(rng: random.Random, timestamp: int) -> Position:
    angle = rng.uniform(0, math.tau)
    radius = rng.uniform(0, 100_000)
    speed_angle = rng.uniform(0, math.tau)
    speed = rng.uniform(0, 5_000)
    velocity = Vector(speed * math.cos(speed_angle), speed * math.sin(speed_angle))
    if rng.random() < 0.1:
        velocity = Vector(0, 0)
    return Position(
        radius * math.cos(angle),
        radius * math.sin(angle),
        velocity,
        TimePoint(timestamp, static=True),
    )


def random_cases() -> List[int]:
    seeds = random.Random(123_456).sample(range(1, 1_000_001), CASES)
    return seeds


def assert_hits_target(
    test_case: unittest.TestCase,
    plan: FlightPlan,
    position: Position,
    target: Position,
    seed: int,
    label: str,
) -> None:
    arrive = plan.apply_to(position)
    predicted = target.predict(arrive.timestamp.usec())
    test_case.assertLess(
        arrive.distance_to(predicted), POSITION_DELTA,
        f"seed {seed} {label} position",
    )
    test_case.assertLess(
        (arrive.velocity - predicted.velocity).abs(), VELOCITY_DELTA,
        f"seed {seed} {label} velocity",
    )


class TestFlightPlan(unittest.TestCase):
    def test_plans_random_ballistic_intercepts(self) -> None:
        # 1. generate randomized kinematic cases
        for seed in random_cases():
            rng = random.Random(seed)

            # 1.1 generate a start and ballistic target
            timestamp = rng.randint(0, 10_000_000)
            position = random_kinematic(rng, timestamp)
            target = random_kinematic(rng, timestamp)
            amax = rng.uniform(5, 100)

            # 1.2 compute and validate the intercept
            plan = approach_to_plan(position, target, amax)
            self.assertIsNotNone(plan, f"seed {seed}")
            assert plan is not None
            assert_hits_target(self, plan, position, target, seed, "intercept")

    def test_plans_in_requested_time_windows(self) -> None:
        # 1. generate randomized kinematic cases
        for seed in random_cases():
            rng = random.Random(seed)

            # 1.1 generate a start, target, and full-thrust plan
            timestamp = rng.randint(0, 10_000_000)
            position = random_kinematic(rng, timestamp)
            target = random_kinematic(rng, timestamp)
            amax = rng.uniform(5, 100)
            fastest = approach_to_plan(position, target, amax)
            self.assertIsNotNone(fastest, f"seed {seed}")
            assert fastest is not None
            duration = fastest.duration_sec()
            if duration <= 0:
                continue

            # 1.2 validate windows slower than full thrust
            for index in range(INNER_WINDOWS):
                # 1.2.1 choose and plan within an inner window
                tmin = duration * rng.uniform(1.2, 2.5)
                tmax = tmin + duration * rng.uniform(0.05, 0.3)
                status, plan = prepare_flight_plan_in_time(
                    position, target, amax, tmin, tmax
                )

                # 1.2.2 validate the window and intercept
                self.assertTrue(status.is_ok(), f"seed {seed} inner {index}: {status}")
                self.assertIsNotNone(plan, f"seed {seed} inner {index}")
                assert plan is not None
                self.assertGreaterEqual(plan.duration_sec(), tmin)
                self.assertLessEqual(plan.duration_sec(), tmax)
                assert_hits_target(self, plan, position, target, seed, f"inner {index}")

            # 1.3 validate a window containing full thrust
            status, plan = prepare_flight_plan_in_time(
                position, target, amax,
                duration * rng.uniform(0.3, 0.9),
                duration * rng.uniform(1.1, 2),
            )
            self.assertTrue(status.is_ok(), f"seed {seed} boundary: {status}")
            self.assertIsNotNone(plan, f"seed {seed} boundary")
            assert plan is not None
            self.assertAlmostEqual(plan.duration_sec(), duration, places=6)
            assert_hits_target(self, plan, position, target, seed, "boundary")

            # 1.4 reject infeasible and exact-duration windows
            too_late_max = duration * rng.uniform(0.1, 0.9)
            late_status, late_plan = prepare_flight_plan_in_time(
                position, target, amax, rng.uniform(0, too_late_max), too_late_max
            )
            self.assertFalse(late_status.is_ok(), f"seed {seed} too late")
            self.assertIsNone(late_plan, f"seed {seed} too late")
            tight = duration * rng.uniform(1.5, 3)
            tight_status, tight_plan = prepare_flight_plan_in_time(
                position, target, amax, tight, tight
            )
            self.assertFalse(tight_status.is_ok(), f"seed {seed} tight")
            self.assertIsNone(tight_plan, f"seed {seed} tight")

    def test_plans_in_requested_delta_v_windows(self) -> None:
        # 1. generate randomized kinematic cases
        for seed in random_cases():
            rng = random.Random(seed)

            # 1.1 generate a start, target, and endpoint plans
            timestamp = rng.randint(0, 10_000_000)
            position = random_kinematic(rng, timestamp)
            target = random_kinematic(rng, timestamp)
            amax = rng.uniform(5, 100)
            fastest = approach_to_plan(position, target, amax)
            slowest = approach_to_plan(position, target, amax / 100)
            self.assertIsNotNone(fastest, f"seed {seed}")
            self.assertIsNotNone(slowest, f"seed {seed} amax/100")
            assert fastest is not None and slowest is not None
            delta_v = fastest.delta_v()
            low_delta_v = slowest.delta_v()
            if delta_v <= 0 or low_delta_v >= delta_v:
                continue
            span = delta_v - low_delta_v

            # 1.2 validate delta-v windows between endpoint plans
            for index in range(INNER_WINDOWS):
                # 1.2.1 choose and plan within an inner window
                dvmin = low_delta_v + span * rng.uniform(0.15, 0.5)
                dvmax = dvmin + span * rng.uniform(0.1, 0.35)
                if dvmax >= delta_v:
                    continue
                status, plan = prepare_flight_plan_in_delta_v(
                    position, target, amax, dvmin, dvmax
                )

                # 1.2.2 validate the delta-v and intercept
                self.assertTrue(status.is_ok(), f"seed {seed} inner {index}: {status}")
                self.assertIsNotNone(plan, f"seed {seed} inner {index}")
                assert plan is not None
                self.assertGreaterEqual(plan.delta_v(), dvmin)
                self.assertLessEqual(plan.delta_v(), dvmax)
                assert_hits_target(self, plan, position, target, seed, f"inner {index}")

            # 1.3 validate a window containing full-thrust delta-v
            status, plan = prepare_flight_plan_in_delta_v(
                position, target, amax,
                delta_v * rng.uniform(0.3, 0.9),
                delta_v * rng.uniform(1.1, 2),
            )
            self.assertTrue(status.is_ok(), f"seed {seed} boundary: {status}")
            self.assertIsNotNone(plan, f"seed {seed} boundary")
            assert plan is not None
            self.assertAlmostEqual(plan.delta_v(), delta_v, places=6)
            assert_hits_target(self, plan, position, target, seed, "boundary")

            # 1.4 reject impossible and exact-delta-v windows
            too_high_min = delta_v * rng.uniform(1.1, 2)
            high_status, high_plan = prepare_flight_plan_in_delta_v(
                position, target, amax,
                too_high_min, too_high_min + delta_v * rng.uniform(0.1, 0.5)
            )
            self.assertFalse(high_status.is_ok(), f"seed {seed} too high")
            self.assertIsNone(high_plan, f"seed {seed} too high")
            tight = low_delta_v + span * rng.uniform(0.3, 0.7)
            tight_status, tight_plan = prepare_flight_plan_in_delta_v(
                position, target, amax, tight, tight
            )
            self.assertFalse(tight_status.is_ok(), f"seed {seed} tight")
            self.assertIsNone(tight_plan, f"seed {seed} tight")
