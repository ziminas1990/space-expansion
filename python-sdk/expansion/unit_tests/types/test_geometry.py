import unittest
import random as rnd
import expansion.types as types
from expansion.unit_tests.utils import random_vector, vectors_equal


class TestCase(unittest.TestCase):

    def test_vector_decompose(self):
        rnd.seed(473758)
        for i in range(1000):
            target = random_vector(10)
            v = random_vector(10)
            a, b = v.decompose(target)
            self.assertTrue(vectors_equal(v, a + b))
            self.assertAlmostEqual(0.0, a.cosa(b))
            self.assertAlmostEqual(1.0, abs(target.cosa(a)))
            self.assertAlmostEqual(0.0, target.cosa(b))

