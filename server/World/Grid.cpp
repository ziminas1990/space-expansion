#include "Grid.h"
#include <assert.h>

namespace world {

Grid* Grid::g_globalGrid = nullptr;


Grid::Grid()
  : m_parentCell(this, 0, 0, 0)
{}

Grid::Grid(uint8_t width, uint32_t cellWidth)
  : m_cellWidth(cellWidth)
  , m_width(width)
  , m_parentCell(this, 0, 0, cellWidth)  // Will be inited later
{
  const int32_t length = static_cast<int32_t>(width * cellWidth);
  m_parentCell = Cell(
        this, -length/2, -length/2, static_cast<uint32_t>(length));

  m_cells.reserve(width * width);
  for (uint32_t i = 0; i < width; ++i) {
    const int32_t y = m_parentCell.bottom() + i * cellWidth;
    for (size_t j = 0; j < width; ++j) {
      const int32_t x = m_parentCell.left() + j * cellWidth;
      m_cells.emplace_back(this, x, y, cellWidth);
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
