import unittest
import random as rnd
import expansion.types as types
from expansion.unit_tests.utils import random_position, vectors_equal
import math


class TestCase(unittest.TestCase):

    def test_position_decompose(self):
        rnd.seed(473758)
        for i in range(10000):
            case_seed = rnd.randint(1, 1000000)
            rnd.seed(case_seed)
            target = random_position(0, 0, 100, 10)
            me = random_position(0, 0, 100, 10)
            a, b = me.decompose(target)

            # Check positioning expectations:
            self.assertAlmostEqual(0.0, me.vector_to(a).cosa(me.vector_to(b)))
            self.assertAlmostEqual(0.0, a.vector_to(target).cosa(b.vector_to(target)))
            self.assertAlmostEqual(0.0, a.vector_to(target).cosa(a.vector_to(me)))
            self.assertAlmostEqual(0.0, b.vector_to(target).cosa(b.vector_to(me)))
            self.assertAlmostEqual(1.0, abs(a.vector_to(target).cosa(target.velocity)))
            self.assertAlmostEqual(0.0, b.vector_to(target).cosa(target.velocity))
            # Check velocities expectations:
            self.assertTrue(vectors_equal(me.velocity, a.velocity + b.velocity))
            self.assertAlmostEqual(0.0, a.velocity.cosa(b.velocity))
            self.assertAlmostEqual(1.0, abs(a.velocity.cosa(target.velocity)))
            self.assertAlmostEqual(0.0, b.velocity.cosa(target.velocity))

