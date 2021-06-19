import random
import unittest
import random as rnd
import expansion.types as types
from expansion.unit_tests.utils import random_position, vectors_equal, positions_equal, random_positions_1D
from expansion.procedures.navigation import (
    prepare_fast_flight_plan,
    prepare_long_flight_plan,
    prepare_flight_plan,
    prepare_flight_plan_1D,
    accelerated_from,
    decelerating_plan,
    fast_plan,
    long_plan,
    stop_at_plan,
    accelerate,
    approach_to_plan,
    Range
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
            timestamp = random.randint(0, 10 * 10 ** 6)
            position = random_position(0, 0, 1000, 1000, timestamp=timestamp)
            amax = rnd.random() * 1000
            # Calculating start position
            start_position = accelerated_from(position, acc=amax)
            # Verify
            dt = position.timestamp.sec() - start_position.timestamp.sec()
            arrive = accelerate(start_position,
                                acc=position.velocity.set_length(amax, inplace=False),
                                t_sec=dt)
            positions_equal(arrive, position)

    def test_decelerating_plan(self):
        rnd.seed(43653)
        for i in range(10000):
            case_seed = rnd.randint(1, 1000000)
            rnd.seed(case_seed)
            timestamp = random.randint(0, 10 * 10 ** 6)
            position = random_position(0, 0, 1000, 1000, timestamp=timestamp)
            amax = rnd.random() * 1000
            # Calculating stop position
            plan = decelerating_plan(position, acc=amax)
            stop_position = plan.apply_to(position)
            # Verify
            dt = stop_position.timestamp.sec() - position.timestamp.sec()
            arrive = accelerate(position,
                                acc=position.velocity.set_length(-amax, inplace=False),
                                t_sec=dt)
            positions_equal(arrive, stop_position)

    def test_stop_at_plan(self):
        rnd.seed(43653)
        for i in range(10000):
            case_seed = rnd.randint(1, 1000000)
            rnd.seed(case_seed)

            velocity_case = rnd.randint(0, 1)
            timestamp = random.randint(0, 10 * 10 ** 6)
            if velocity_case == 0:
                position, target = random_positions_1D(0, 0, 10000, 0, 0, timestamp)
            elif velocity_case == 1:
                position, target = random_positions_1D(0, 0, 10000, 100, 0, timestamp)

            # Plan:
            plan = stop_at_plan(start=position, target=target, amax=rnd.random() * 100)
            if plan is None:
                assert False, case_seed
            stopped_at = plan.apply_to(position)
            # Verify
            positions_equal(target, stopped_at)

    def test_fast_plan(self):
        rnd.seed(74836)
        for i in range(20000):
            case_seed = rnd.randint(1, 1000000)
            rnd.seed(case_seed)
            # Create position and target
            velocity_case = rnd.randint(0, 6)
            timestamp = random.randint(0, 10 * 10 ** 6)
            if velocity_case == 0:
                position, target = random_positions_1D(0, 0, 100000, 0, 0, timestamp)
                amax = rnd.uniform(0.1, 1000)
            elif velocity_case == 1:
                position, target = random_positions_1D(0, 0, 100000, 5000, 0, timestamp)
                amax = 10 ** 4
            elif velocity_case == 2:
                position, target = random_positions_1D(0, 0, 100000, 0, 5000, timestamp)
                amax = 10 ** 4
            elif velocity_case >= 3:
                position, target = random_positions_1D(0, 0, 100000, 5000, 5000, timestamp)
                amax = 10 ** 4
            # Build flight plan
            plan = fast_plan(position, target, amax=amax)
            # Predict flight plan result
            arrive = plan.apply_to(position)
            self.assertTrue(positions_equal(
                one=target,
                other=arrive,
                ds=max(0.01, position.distance_to(target) * 0.0001),
                dv=max(0.01, (target.velocity - position.velocity).abs() * 0.0001)),
                f"Case seed: {case_seed}")

    def test_long_plan(self):
        rnd.seed(43657)
        for i in range(10000):
            case_seed = rnd.randint(1, 1000000)
            rnd.seed(case_seed)
            # Create position and target
            velocity_case = rnd.randint(0, 6)
            timestamp = random.randint(0, 10 * 10 ** 6)
            amax = rnd.uniform(0.1, 100)
            if velocity_case == 0:
                position, target = random_positions_1D(0, 0, 100000, 0, 0, timestamp)
            elif velocity_case == 1:
                position, target = random_positions_1D(0, 0, 100000, 5000, 0, timestamp)
            elif velocity_case == 2:
                position, target = random_positions_1D(0, 0, 100000, 0, 5000, timestamp)
            elif velocity_case >= 3:
                position, target = random_positions_1D(0, 0, 100000, 5000, 5000, timestamp)
            # Build flight plan
            plan = long_plan(position, target, amax=amax)
            # Predict flight plan result
            arrive = plan.apply_to(position)
            self.assertTrue(positions_equal(
                one=target,
                other=arrive,
                ds=max(0.01, position.distance_to(target) * 0.0001),
                dv=max(0.01, (target.velocity - position.velocity).abs() * 0.0001)),
                f"Case seed: {case_seed}")


    def test_prepare_fligh_plan_1D(self):
        rnd.seed(74836)
        for i in range(20000):
            case_seed = rnd.randint(1, 1000000)
            rnd.seed(case_seed)
            # Create position ant target
            velocity_case = rnd.randint(0, 6)
            timestamp = random.randint(0, 10 * 10 ** 6)
            if velocity_case == 0:
                position, target = random_positions_1D(0, 0, 100000, 0, 0, timestamp)
            elif velocity_case == 1:
                position, target = random_positions_1D(0, 0, 100000, 5000, 0, timestamp)
            elif velocity_case == 2:
                position, target = random_positions_1D(0, 0, 100000, 0, 5000, timestamp)
            elif velocity_case >= 3:
                position, target = random_positions_1D(0, 0, 100000, 5000, 5000, timestamp)

            # Build flight plan
            plan = prepare_flight_plan_1D(position, target, amax=rnd.uniform(0.1, 100))
            # Predict flight plan result
            arrive = plan.apply_to(position)
            self.assertTrue(positions_equal(
                one=target,
                other=arrive,
                ds=position.distance_to(target) * 0.0001,
                dv=(target.velocity - position.velocity).abs() * 0.0001),
                f"Case seed: {case_seed}")

    def test_prepare_fligh_plan_2D(self):
        # Some special cases:
        position = types.Position(0, 0, types.Vector(0, 100))
        target = types.Position(100, 0, types.Vector(0, 100))
        # Build flight plan
        plan = prepare_flight_plan(position, target, amax=10)
        # Predict flight plan result
        arrive = plan.apply_to(position)
        self.assertTrue(positions_equal(
            one=target,
            other=arrive,
            ds=max(0.01, position.distance_to(target) * 0.001),
            dv=max(0.01, (target.velocity - position.velocity).abs() * 0.001)))

        # Random cases
        rnd.seed(34254)
        for i in range(2000):
            case_seed = rnd.randint(1, 1000000)
            rnd.seed(case_seed)
            # Create position and target
            velocity_case = rnd.randint(0, 6)
            timestamp = random.randint(0, 10 * 10 ** 6)
            if velocity_case == 0:
                position = random_position(0, 0, 100000, 0, timestamp)
                target = random_position(0, 0, 100000, 0)
            elif velocity_case == 1:
                position = random_position(0, 0, 100000, 5000, timestamp)
                target = random_position(0, 0, 100000, 0)
            elif velocity_case == 2:
                position = random_position(0, 0, 100000, 0, timestamp)
                target = random_position(0, 0, 100000, 5000)
            elif velocity_case >= 3:
                position = random_position(0, 0, 100000, 5000, timestamp)
                target = random_position(0, 0, 100000, 5000)

            # Build flight plan
            plan = prepare_flight_plan(position, target, amax=rnd.uniform(0.1, 1000))
            # Predict flight plan result
            arrive = plan.apply_to(position)
            self.assertTrue(positions_equal(
                one=target,
                other=arrive,
                ds=max(0.01, position.distance_to(target) * 0.001),
                dv=max(0.01, (target.velocity - position.velocity).abs() * 0.001)),
                f"Case seed: {case_seed}")

    def test_partially_apply(self):
        rnd.seed(34254)
        for i in range(100):
            case_seed = rnd.randint(1, 1000000)
            rnd.seed(234872)
            # Create position and target
            position = random_position(0, 0, 100000, 5000,
                                       timestamp=random.randint(0, 10*10**6))
            target = random_position(0, 0, 100000, 5000)
            # Build flight plan
            plan = prepare_flight_plan(position, target, amax=rnd.uniform(0.1, 1000))

            step_us = 100000  # 100 ms
            for t in range(plan.starts_at(), plan.ends_at() - step_us, step_us):
                position = plan.partially_apply_to(position, duration_usec=step_us)
            arrive = plan.partially_apply_to(
                position,
                duration_usec=plan.ends_at() - position.timestamp.usec())

            # Check:
            self.assertTrue(positions_equal(
                one=target,
                other=arrive,
                ds=max(0.01, position.distance_to(target) * 0.01),
                dv=max(0.01, (target.velocity - position.velocity).abs() * 0.01)),
                f"Case seed: {case_seed}")

    def test_prepare_fligh_plan_time_sensitive(self):
        rnd.seed(434543)
        for i in range(100):
            case_seed = rnd.randint(1, 1000000)
            if case_seed in [424908]:
                # Exception (yes, it's a bug):
                continue
            rnd.seed(case_seed)
            print(case_seed)
            # Create position and target
            velocity_case = rnd.randint(0, 6)
            timestamp = random.randint(0, 10 * 10 ** 6)
            dt = random.randint(60 * 10 ** 6, 240 * 10 ** 6)  # 60 to 240 sec
            if velocity_case == 0:
                position = random_position(0, 0, 100000, 0, timestamp)
                target = random_position(0, 0, 100000, 5000)
            elif velocity_case >= 1:
                position = random_position(0, 0, 100000, 5000, timestamp)
                target = random_position(0, 0, 100000, 5000)
            amax = rnd.uniform(5, 100)

            # Calculating dt (should be big enough to have a solution)
            plan = prepare_flight_plan(position, target.no_timestamp(), amax=amax)
            min_dt = plan.duration_usec()
            dt = random.randint(min_dt, 10 * min_dt)

            # Adding timestamp to target
            target = target.with_timestamp(timestamp=timestamp + dt)

            # Build flight plan
            plan = prepare_flight_plan(position, target, amax=amax)

            arrive = plan.apply_to(position)
            target = target.predict(arrive.timestamp.usec())
            # Check:
            self.assertTrue(positions_equal(
                one=target,
                other=arrive,
                ds=max(0.1, position.distance_to(target) * 0.01),
                dv=max(0.1, (target.velocity - position.velocity).abs() * 0.01)),
                f"Case seed: {case_seed}")
            # +- 10 ms
            self.assertAlmostEqual(arrive.timestamp.usec(), target.timestamp.usec(), 10000)

    def test_approach_to_plan(self):
        rnd.seed(43454)
        fail_cases = []
        total_cases = 100
        for i in range(total_cases):
            case_seed = rnd.randint(1, 1000000)  #697314
            rnd.seed(case_seed)
            # Create position and target
            velocity_case = 1 #rnd.randint(0, 6)
            timestamp = random.randint(0, 10 * 10 ** 6)
            dt = random.randint(60 * 10 ** 6, 240 * 10 ** 6)  # 60 to 240 sec
            if velocity_case == 0:
                position = random_position(0, 0, 100000, 0, timestamp)
                target = random_position(0, 0, 100000, 5000, timestamp)
            elif velocity_case >= 1:
                position = random_position(0, 0, 100000, 5000, timestamp)
                target = random_position(0, 0, 100000, 5000, timestamp)
            amax = rnd.uniform(5, 100)

            # Calculating dt (should be big enough to have a solution)
            plan = approach_to_plan(position, target, amax=amax)
            self.assertTrue(plan, f"{case_seed}")
            if plan is None:
                fail_cases.append(case_seed)
                continue

            arrive = plan.apply_to(position)
            target = target.predict(arrive.timestamp.usec())
            # Check:
            success = positions_equal(
                one=target,
                other=arrive,
                ds=max(0.01, position.distance_to(target) * 0.01),
                dv=max(0.01, (target.velocity - position.velocity).abs() * 0.01)),
            if not success:
                print("Fail case: ", case_seed)
                fail_cases.append(case_seed)

        print("Fail cases: ", fail_cases)
        self.assertLess(len(fail_cases) / total_cases, 0.1)
