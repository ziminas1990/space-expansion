#include "Grid.h"
#include <assert.h>

namespace world {


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

} // namespace world
