#pragma once

#include <random>

namespace utils {

class RandomSequence
{
public:
  RandomSequence(unsigned int nInitialPattern)
    : m_nNextPattern(nInitialPattern)
  {}

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

private:
  unsigned int m_nNextPattern;
};

} // namespace utils
