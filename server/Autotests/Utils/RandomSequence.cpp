#include <Utils/RandomSequence.h>
#include <vector>
#include <gtest/gtest.h>

using namespace utils;

TEST(RandomSequence, yield)
{
  const size_t nTotal = 1000;

  std::vector<uint32_t> expectedA(nTotal);
  std::vector<uint32_t> expectedB(nTotal);
  std::vector<uint32_t> expectedC(nTotal);

  {
    RandomSequence generator(static_cast<unsigned int>('A'));
    for (size_t i = 0; i < nTotal; ++i) {
      expectedA[i] = generator.yield();
    }
  }
  {
    RandomSequence generator(static_cast<unsigned int>('B'));
    for (size_t i = 0; i < nTotal; ++i) {
      expectedB[i] = generator.yield();
    }
  }
  {
    RandomSequence generator(static_cast<unsigned int>('C'));
    for (size_t i = 0; i < nTotal; ++i) {
      expectedC[i] = generator.yield();
    }
  }

  // Should have the same sequences if yield in parallel
  {
    RandomSequence generatorA(static_cast<unsigned int>('A'));
    RandomSequence generatorB(static_cast<unsigned int>('B'));
    RandomSequence generatorC(static_cast<unsigned int>('C'));
    for (size_t i = 0; i < nTotal; ++i) {
      ASSERT_EQ(expectedA[i], generatorA.yield());
      ASSERT_EQ(expectedB[i], generatorB.yield());
      ASSERT_EQ(expectedC[i], generatorC.yield());
    }
  }
}

TEST(RandomSequence, yield64)
{
  const size_t nTotal = 1000;

  std::vector<uint64_t> expectedA(nTotal);
  std::vector<uint64_t> expectedB(nTotal);
  std::vector<uint64_t> expectedC(nTotal);

  {
    RandomSequence generator(static_cast<unsigned int>('A'));
    for (size_t i = 0; i < nTotal; ++i) {
      expectedA[i] = generator.yield64();
    }
  }
  {
    RandomSequence generator(static_cast<unsigned int>('B'));
    for (size_t i = 0; i < nTotal; ++i) {
      expectedB[i] = generator.yield64();
    }
  }
  {
    RandomSequence generator(static_cast<unsigned int>('C'));
    for (size_t i = 0; i < nTotal; ++i) {
      expectedC[i] = generator.yield64();
    }
  }

  // Should have the same sequences if yield in parallel
  {
    RandomSequence generatorA(static_cast<unsigned int>('A'));
    RandomSequence generatorB(static_cast<unsigned int>('B'));
    RandomSequence generatorC(static_cast<unsigned int>('C'));
    for (size_t i = 0; i < nTotal; ++i) {
      ASSERT_EQ(expectedA[i], generatorA.yield64());
      ASSERT_EQ(expectedB[i], generatorB.yield64());
      ASSERT_EQ(expectedC[i], generatorC.yield64());
    }
  }
}

TEST(RandomSequence, generate)
{
  const size_t nTotal = 1000;
  const std::pair<int, int> limits [] = {
    {0, 100},
    {-100, 100},
    {-200, -100},
    {200, 500},
    {0, 1}
  };

  for (const std::pair<int, int> limit: limits) {
    RandomSequence generator(3234);
    std::vector<int> data = generator.generate(
          nTotal, limit.first, limit.second);
    for (int element: data) {
      ASSERT_LE(limit.first, element);
      ASSERT_GE(limit.second, element);
    }
  }
}
