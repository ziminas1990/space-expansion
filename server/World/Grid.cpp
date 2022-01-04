#include "Grid.h"
#include <assert.h>

namespace world {

Grid* Grid::g_globalGrid = nullptr;


Grid::Grid()
  : m_parentCell(this, 0, 0, 0)
{}

Grid::Grid(uint16_t nWidth, uint32_t nCellWidth)
  : m_cellWidth(nCellWidth)
  , m_width(nWidth)
  , m_parentCell(this, 0, 0, nCellWidth)  // Will be inited later
{
  const uint64_t nLength = static_cast<uint64_t>(nWidth * nCellWidth);
  assert(nLength < 0xFFFFFFFF);
  const int32_t nHalfSize = static_cast<int32_t>(nLength / 2);

  m_parentCell = Cell(
        this, -nHalfSize, -nHalfSize, static_cast<uint32_t>(nLength));

  m_cells.reserve(nWidth * nWidth);
  for (uint32_t i = 0; i < nWidth; ++i) {
    const int32_t y = m_parentCell.bottom() + i * nCellWidth;
    for (size_t j = 0; j < nWidth; ++j) {
      const int32_t x = m_parentCell.left() + j * nCellWidth;
      m_cells.emplace_back(this, x, y, nCellWidth);
    }
  }
}

Grid::iterator Grid::begin() const
{
  return iterator(this, 0, m_width, m_width);
}

Grid::iterator Grid::end() const
{
  return iterator(this, m_cells.size(), 0, 0);
}

Grid::iterator::iterator(
    const Grid *pOwner,
    size_t      nBegin,
    size_t      nWidth,
    size_t      nHeight)
  : m_nRow(nBegin / pOwner->m_width),
    m_nColumn(nBegin % pOwner->m_width),
    m_nPos(nBegin),
    m_nColumnLeft(m_nColumn),
    m_nColumnRight(m_nColumn + nWidth),
    m_nRowEnd(m_nRow + nHeight),
    m_nRowLength(pOwner->m_width),
    m_pOwner(pOwner)
{}

Grid::iterator& Grid::iterator::operator++() {
  ++m_nColumn;
  ++m_nPos;
  if (m_nColumn < m_nColumnRight) {
    return *this;
  }
  // Jump to the new row
  m_nColumn = m_nColumnLeft;
  ++m_nRow;
  if (m_nRow < m_nRowEnd) {
    m_nPos = m_nRow * m_nRowLength + m_nColumn;
    return *this;
  }
  // End of area is reached
  m_nPos    = m_pOwner->m_cells.size();
  m_nColumn = m_pOwner->m_width;
  m_nRow    = m_pOwner->m_width;
  return *this;
}

} // namespace world
