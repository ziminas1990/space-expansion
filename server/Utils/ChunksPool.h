#pragma once

#include <vector>
#include <stdint.h>

namespace utils {

class ChunksPool
{
public:
  ChunksPool(size_t nSmallChunksCount  = 512,
             size_t nMediumChunksCount = 64,
             size_t nHugeChunksCount   = 16,
             size_t nSmallChunksSize   = 256,
             size_t nMediumChunksSize  = 2048,
             size_t nHugeChunksSize    = 16384)
    : m_nSmallChunksSize(nSmallChunksSize),   m_nSmallChunksCount(nSmallChunksCount),
      m_nSmallArenaSize(nSmallChunksSize * nSmallChunksCount),
      m_nMediumChunksSize(nMediumChunksSize), m_nMediumChunksCount(nMediumChunksCount),
      m_nMediumArenaSize(nMediumChunksSize * nMediumChunksCount),
      m_nHugeChunksSize(nHugeChunksSize),     m_nHugeChunksCount(nHugeChunksCount),
      m_nHugeArenaSize(nHugeChunksSize * nHugeChunksCount)
  {
    size_t nTotal = m_nSmallArenaSize + m_nMediumArenaSize + m_nHugeArenaSize;
    m_pArena = new uint8_t[nTotal];

    m_smallChunks.reserve(m_nSmallChunksCount);
    m_mediumChunks.reserve(m_nMediumChunksCount);
    m_hugeChunks.reserve(m_nHugeChunksCount);
    uint8_t* pChunk = m_pArena;
    for(size_t i = 0; i < m_nSmallChunksCount; ++i, pChunk += m_nSmallChunksSize)
      m_smallChunks.push_back(pChunk);
    for(size_t i = 0; i < m_nMediumChunksCount; ++i, pChunk += m_nMediumChunksSize)
      m_mediumChunks.push_back(pChunk);
    for(size_t i = 0; i < m_nHugeChunksCount; ++i, pChunk += m_nHugeChunksSize)
      m_hugeChunks.push_back(pChunk);
  }

  ChunksPool(ChunksPool const& other) = delete;
  ChunksPool(ChunksPool&& other)      = delete;

  ~ChunksPool() { delete [] m_pArena; }

  uint8_t* get(size_t nSize)
  {
    uint8_t* pChunk = nullptr;
    if (nSize <= m_nSmallChunksSize && m_smallChunks.size()) {
      pChunk = m_smallChunks.back();
      m_smallChunks.pop_back();
    } else if (nSize <= m_nMediumChunksSize && m_mediumChunks.size()) {
      pChunk = m_mediumChunks.back();
      m_mediumChunks.pop_back();
    } else if (nSize <= m_nHugeChunksSize && m_hugeChunks.size()) {
      pChunk = m_hugeChunks.back();
      m_hugeChunks.pop_back();
    }
    return pChunk;
  }

  bool release(uint8_t* pChunk)
  {
    if (pChunk < m_pArena)
      return false;
    size_t nOffset = static_cast<size_t>(pChunk - m_pArena);
    if (nOffset < m_nSmallArenaSize) {
      m_smallChunks.push_back(pChunk);
      return true;
    } else if (nOffset < m_nSmallArenaSize + m_nMediumChunksSize) {
      m_mediumChunks.push_back(pChunk);
      return true;
    } else if (nOffset < m_nSmallArenaSize + m_nMediumChunksSize + m_nHugeArenaSize) {
      m_hugeChunks.push_back(pChunk);
      return true;
    }
    return false;
  }

private:
  std::vector<uint8_t*> m_smallChunks;
  std::vector<uint8_t*> m_mediumChunks;
  std::vector<uint8_t*> m_hugeChunks;

  size_t m_nSmallChunksSize   = 0;
  size_t m_nSmallChunksCount  = 0;
  size_t m_nSmallArenaSize    = 0;
  size_t m_nMediumChunksSize  = 0;
  size_t m_nMediumChunksCount = 0;
  size_t m_nMediumArenaSize   = 0;
  size_t m_nHugeChunksSize    = 0;
  size_t m_nHugeChunksCount   = 0;
  size_t m_nHugeArenaSize     = 0;
  uint8_t* m_pArena           = nullptr;
};

} // namespace utils
