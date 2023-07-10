#include <gtest/gtest.h>
#include <stdint.h>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <Utils/IndexedVector.hpp>

using namespace utils;

struct TestItem {
  uint32_t hash;
};

uint32_t hash(const TestItem& item) {
  return item.hash;
}

static std::vector<TestItem> removeItems(
  const std::vector<TestItem>& arr,
  std::function<bool(const TestItem&)> predicate)
{
    std::vector<TestItem> filtered;
    for (const TestItem& item : arr) {
      if (!predicate(item)) {
        filtered.push_back(item);
      }
    }
    return filtered;
};

static std::vector<TestItem> filterItems(
  const std::vector<TestItem>& arr,
  std::function<bool(const TestItem&)> predicate)
{
    std::vector<TestItem> filtered;
    for (const TestItem& item : arr) {
      if (predicate(item)) {
        filtered.push_back(item);
      }
    }
    return filtered;
};

std::vector<TestItem> buildRandomVector(uint32_t max, unsigned seed) {
  std::vector<TestItem> items;
  for(uint32_t i = 0; i < max; ++i) {
    items.push_back(TestItem{i});
  }
  std::srand(seed);
  std::random_shuffle(items.begin(), items.end());
  return items;
}

TEST(IndexedVector, push_and_pop) {
  utils::IndexedVector<TestItem, uint32_t> vector(hash);

  constexpr unsigned seed = 372863;

  std::vector<TestItem> items = buildRandomVector(128, seed);

  for (size_t i = 0; i < items.size(); ++i) {
    vector.push(items[i]);
    for (size_t j = 0; j < items.size(); ++j) {
      const TestItem  probablyExpected = items[j];
      const TestItem* extracted        = vector.get(probablyExpected.hash);
      if (j <= i) {
        ASSERT_TRUE(extracted);
        ASSERT_EQ(probablyExpected.hash, extracted->hash);
      } else {
        ASSERT_EQ(nullptr, extracted);
      }
    }
  }

  std::srand(seed);
  std::random_shuffle(items.begin(), items.end());

  for (size_t i = 0; i < items.size(); ++i) {
    {
      const TestItem& expected = items[i];
      TestItem extracted;
      ASSERT_TRUE(vector.pop(expected.hash, extracted));
      ASSERT_EQ(expected.hash, extracted.hash);
    }
    // Check that previously poped items can't be poped anymore
    for (size_t j = 0; j <= i; ++j) {
      const TestItem& expected = items[j];
      TestItem extracted;
      ASSERT_FALSE(vector.pop(expected.hash, extracted)) << "for i=" << i << ", j=" << j;
    }
  }
}

TEST(IndexedVector, remove) {
  utils::IndexedVector<TestItem, uint32_t> vector(hash);

  const std::vector<TestItem> items = buildRandomVector(128, 483728);

  for (size_t i = 0; i < items.size(); ++i) {
    vector.push(items[i]);
  }

  std::vector<TestItem> expectedItems = items;

  for (uint32_t prime: {19, 17, 13, 11, 7, 5, 3, 2}) {
    const auto predicate = [prime](const TestItem& item) {
      return item.hash % prime == 0;
    };

    const std::vector<TestItem> removedItems = filterItems(expectedItems, predicate);
    expectedItems = removeItems(expectedItems, predicate);

    const size_t nTotalRemoved = vector.remove(predicate);
    ASSERT_EQ(removedItems.size(), nTotalRemoved);

    for (const TestItem& item: expectedItems) {
      ASSERT_TRUE(vector.get(item.hash));
    }

    for (const TestItem& item: removedItems) {
      ASSERT_FALSE(vector.get(item.hash));
    }
  }
}