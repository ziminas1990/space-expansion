#include <Utils/IdArray.h>
#include <Utils/RandomSequence.h>

#include <boost/fiber/barrier.hpp>
#include <gtest/gtest.h>

inline bool is_special(int id)
{
  for (int prime: {3, 5, 7, 11}) {
    if (id % prime == 0) {
      return true;
    }
  }
  return false;
}

TEST(IdArrayTests, breath)
{
  utils::RandomSequence generator(37284);
  utils::IdArray<int> array;

  int summ = 0;
  for (int i = 0; i < 1000; ++i) {
    int id = generator.yield();
    array.push(id);
    summ += id;
  }

  int id;
  size_t index;
  array.begin();
  while (array.getNextId(id, index)) {
    summ -= id;
  }
  ASSERT_EQ(0, summ);
}

TEST(IdArrayTests, remove_while_iterating)
{
  utils::IdArray<int> array;

  const int primes[] = {11, 7, 5, 3, 2};

  for (int i = 100; i < 10000; ++i) {
    array.push(i);
  }

  for (int prime: primes) {
    int expectedSum  = 0;
    int expectedSize = 0;
    int id;
    size_t index;
    array.begin();
    while (array.getNextId(id, index)) {
      if (id % prime) {
        expectedSum += id;
        ++expectedSize;
      } else {
        array.dropIndex(index);
      }
    }

    int sum = 0;
    array.begin();
    while (array.getNextId(id, index)) {
      ASSERT_NE(0, id % prime);
      sum += id;
    }
    ASSERT_EQ(expectedSum, sum);
    ASSERT_EQ(expectedSize, array.size());
  }
}

TEST(IdArrayTests, remove_while_iterating_mt)
{
  for (size_t totalThreads = 1; totalThreads < 64; ++totalThreads) {
    boost::fibers::barrier barrier(totalThreads + 1);

    utils::IdArray<int> array;
    for (int i = 100; i < 10000; ++i) {
      array.push(i);
    }

    for (int prime: {3, 5, 7, 11}) {

      std::atomic_int summ;
      summ.store(0);
      auto remove_specials = [&array, &barrier, &summ, &prime]() {
        barrier.wait();
        size_t index;
        int id;
        while (array.getNextId(id, index)) {
          if (id % prime == 0) {
            array.dropIndex(index);
          } else {
            summ.fetch_add(id);
          }
        }
      };

      std::vector<std::thread> threads;
      for (size_t i = 0; i < totalThreads; ++i) {
        // Spawn a number of threads, that will remove from array all values,
        // devidable by 'prime'
        threads.emplace_back(remove_specials);
      }

      // Prepare array for iteration
      array.begin();
      // Run all threads (they are waiting on the barrier)
      barrier.wait();
      // Wait all threads to finish
      for (auto& thread : threads) {
        thread.join();
      }

      // Check 'array' content: all special ids must be removed
      array.begin();
      int expectedSum = 0;
      {
        size_t index;
        int id;
        while (array.getNextId(id, index)) {
          ASSERT_FALSE(id % prime == 0)
              << "id = " << id << ", totalThreads = " << totalThreads;
          expectedSum += id;
        }
      }
      ASSERT_EQ(expectedSum, summ.load()) << "total threads = " << totalThreads;
    }
  }
}
