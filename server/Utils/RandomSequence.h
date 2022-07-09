#pragma once

#include <vector>
#include <assert.h>
#include <random>

namespace utils {

class RandomSequence
{
public:
  RandomSequence(unsigned int nInitialPattern)
    : m_nNextPattern(nInitialPattern)
  {}

  uint16_t yield16() {
    std::srand(m_nNextPattern);
    m_nNextPattern = static_cast<unsigned int>(std::rand());
    return static_cast<uint16_t>(std::rand());
  }

  uint32_t yield() {
    std::srand(m_nNextPattern);
    m_nNextPattern = static_cast<unsigned int>(std::rand());
    return static_cast<uint32_t>(std::rand());
  }

  uint64_t yield64() {
    std::srand(m_nNextPattern);
    m_nNextPattern = static_cast<unsigned int>(std::rand());
    uint32_t lowBytes = static_cast<uint32_t>(std::rand());
    uint64_t hiBytes = static_cast<uint32_t>(std::rand());
    return (hiBytes << 32) + lowBytes;
  }

  std::vector<int> generate(size_t nTotal, int nLeftBound, int nRightBound)
  {
    assert(nLeftBound < nRightBound);
    const int range = nRightBound - nLeftBound + 1;

    std::srand(m_nNextPattern);
    m_nNextPattern = static_cast<unsigned int>(std::rand());

    std::vector<int> result(nTotal);
    for (size_t i = 0; i < nTotal; ++i) {
      result[i] = nLeftBound + std::rand() % range;
    }
    return result;
  }

private:
  unsigned int m_nNextPattern;
};

} // namespace utils
