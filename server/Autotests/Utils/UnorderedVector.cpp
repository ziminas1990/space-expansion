#include <gtest/gtest.h>
#include <set>
#include <vector>
#include <Utils/RandomSequence.h>
#include <Utils/UnorderedVector.h>

using namespace utils;

template<typename T, typename Iterable>
T calculateSum(const Iterable& container)
{
  T total = 0;
  for (const T& element: container) {
    total += element;
  }
  return total;
}

TEST(UnorderedVector, push)
{
  const int seed = 38276;
  RandomSequence generator(seed);

  const std::vector<int> elements = generator.generate(1000, 0, 200);
  const int expectedSumm = calculateSum<int>(elements);

  const std::set<int> uniqueElements(elements.begin(), elements.end());
  const int expectedUniqueSum = calculateSum<int>(uniqueElements);

  // Duplicates are allowed
  {
    UnorderedVector<int> unordered;
    for (int item: elements) {
      unordered.push(item, false);
    }
    ASSERT_EQ(expectedSumm, calculateSum<int>(unordered.data()));
  }

  // Duplicates are NOT allowed
  {
    UnorderedVector<int> unordered;
    for (int item: elements) {
      unordered.push(item);
    }
    ASSERT_EQ(expectedUniqueSum, calculateSum<int>(unordered.data()));
  }
}

TEST(UnorderedVector, remove_while_iterating)
{
  for (bool checkIfExist: {true, false}) {
    const int seed = 38276;
    RandomSequence generator(seed);

    int primes[] = {3, 5, 7, 11, 13, 2};

    const auto elements = generator.generate(1000, -50, 200);

    UnorderedVector<int> array;
    int expectedSumm = 0;
    for (int item: elements) {
      if (array.push(item, checkIfExist)) {
        expectedSumm += item;
      }
    }

    for (int prime: primes) {
      int removedSum = 0;
      int leftSum = 0;
      bool lRemoved = false;
      for (size_t i = 0; i < array.size(); i += lRemoved ? 0 : 1) {
        lRemoved = 0 == (array[i] % prime);
        if (lRemoved) {
          removedSum += array[i];
          array.remove(i);
        } else {
          leftSum += array[i];
        }
      }
      ASSERT_EQ(expectedSumm, leftSum + removedSum) << prime;
      expectedSumm = leftSum;
    }

    for (int item: array.data()) {
      for (int prime: primes) {
        ASSERT_NE(0, item % prime) << item << " % " << prime;
      }
    }
  }
}

TEST(UnorderedVector, remove_all)
{
  const int seed = 38276;
  const int leftBound = -50;
  const int rightBound = 200;

  for (bool checkIfExist: {true, false}) {
    RandomSequence generator(seed);

    const auto elements = generator.generate(1000, leftBound, rightBound);

    UnorderedVector<int> array;
    int totalSumm = 0;
    for (int item: elements) {
      if (array.push(item, checkIfExist)) {
        totalSumm += item;
      }
    }

    for (int i = leftBound; i <= rightBound; ++i) {
      size_t totalRemoved = array.removeAll(i);
      totalSumm -= static_cast<int>(totalRemoved) * i;
      ASSERT_EQ(totalSumm, calculateSum<int>(array.data()));
      for (int item: array.data()) {
        ASSERT_NE(i, item);
      }
    }

    ASSERT_EQ(size_t(0), array.size());
  }
}
