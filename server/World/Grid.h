#pragma once

#include <stdint.h>
#include <unordered_map>
#include <memory>
#include <vector>
#include <assert.h>

#include <Utils/Mutex.h>
#include <Utils/UnorderedVector.h>
#include <Geometry/Rectangle.h>

namespace world {

class Grid;


class Cell
{
private:
  Grid     *m_pOwner;
  int32_t   m_x;
  int32_t   m_y;
  uint32_t  m_width;

  std::unique_ptr<utils::Mutex>    m_pMutex;
  utils::UnorderedVector<uint32_t> m_objectsIds;

public:
  Cell(Grid* pOwner, int32_t x, int32_t y, uint32_t width)
    : m_pOwner(pOwner),
      m_x(x),
      m_y(y),
      m_width(width),
      m_pMutex(new utils::Mutex())
  {}

  int32_t left()   const { return m_x; }
  int32_t right()  const { return m_x + static_cast<int32_t>(m_width); }
  int32_t bottom() const { return m_y; }
  int32_t top()    const { return m_y + static_cast<int32_t>(m_width); }

  template<typename NumericType>
  bool contains(NumericType x, NumericType y) const {
    return (x >= m_x) && (x - m_x) < static_cast<int32_t>(m_width)
        && (y >= m_y) && (y - m_y) < static_cast<int32_t>(m_width);
  }

  template<typename NumericType>
  void add(uint32_t nObjectId, NumericType x, NumericType y) {
    assert(contains(x, y));
    assert(!m_objectsIds.has(nObjectId));
    m_objectsIds.push(nObjectId);
  }

  const utils::UnorderedVector<uint32_t>& getObjects() const {
    return m_objectsIds;
  }

  template<typename NumericType>
  Cell* track(uint32_t nObjectId, NumericType x, NumericType y);

  geometry::Rectangle asRect() const {
    return geometry::Rectangle(
          geometry::Point(left(), top()),
          geometry::Point(right(), bottom()));
  }
};

class Grid {
  uint32_t          m_cellWidth = 0;
  uint8_t           m_width     = 0;
  std::vector<Cell> m_cells;
  Cell              m_parentCell;

  template<typename NumericType>
  size_t indexOf(NumericType x, NumericType y) const {
    const int32_t i = (x - m_parentCell.left()) / m_cellWidth;
    const int32_t j = (y - m_parentCell.bottom()) / m_cellWidth;
    return static_cast<size_t>(j * m_width + i);
  }

  static Grid* g_globalGrid;

public:
  struct iterator {
    using iterator_category = std::input_iterator_tag;
    using value_type        = Cell;
    using pointer           = const value_type*;
    using reference         = const value_type&;

    iterator(const Grid* pOwner,
             size_t      nBegin,
             size_t      nWidth,
             size_t      nHeight);
    // Create an iterator that starts from the cell with the specified 'nBegin'
    // index and iterate throught the region of the specified 'nWidth' columns
    // and 'nHieght' rows.

  private:
    // Current position
    size_t m_nRow;
    size_t m_nColumn;
    size_t m_nPos;
    // Area parameters
    size_t m_nColumnLeft;
    size_t m_nColumnRight;
    size_t m_nRowEnd;
    size_t m_nRowLength;
    // Owner
    const Grid* m_pOwner;

  public:
    reference operator*()  const {
      assert(m_nPos < m_pOwner->m_cells.size());
      return m_pOwner->m_cells[m_nPos];
    }

    iterator& operator++();

    bool operator==(const iterator& other) const {
      assert(m_pOwner == other.m_pOwner);
      return m_nPos == other.m_nPos;
    }

    bool operator!=(const iterator& other) const {
      assert(m_pOwner == other.m_pOwner);
      return m_nPos != other.m_nPos;
    }
  };

public:
  static void setGlobal(Grid* pGlobalGrid) {
    g_globalGrid = pGlobalGrid;
  }
  static Grid* getGlobal() { return g_globalGrid; }

  Grid();
  Grid(uint8_t width, uint32_t cellWidth);

  int32_t left()   const { return m_parentCell.left(); }
  int32_t right()  const { return m_parentCell.right(); }
  int32_t bottom() const { return m_parentCell.bottom(); }
  int32_t top()    const { return m_parentCell.top(); }

  template<typename NumericType>
  const Cell* getCell(NumericType x, NumericType y) const {
    return contains(x, y) ? &m_cells[indexOf(x, y)] : nullptr;
  }

  template<typename NumericType>
  Cell* getCell(NumericType x, NumericType y) {
    return contains(x, y) ? &m_cells[indexOf(x, y)] : nullptr;
  }

  template<typename NumericType>
  Cell* add(uint32_t nObjectId, NumericType x, NumericType y) {
    Cell* pCell = getCell(x, y);
    if (pCell) {
      pCell->add(nObjectId, x, y);
    }
    return pCell;
  }

  const std::vector<Cell>& cells() const { return m_cells; }

  template<typename NumericType>
  bool contains(NumericType x, NumericType y) const {
    return m_parentCell.contains(x, y);
  }

  iterator begin() const;
  iterator end() const;

  template<typename NumericType>
  iterator range(NumericType x, NumericType y,
                 NumericType width, NumericType height) const;
  // Return iterator, that iterates through all cells, covered by a rectangle
  // of size 'width' and 'height' with it's top left cornet at the specified
  // 'x' and 'y' position.

  geometry::Rectangle asRect() const {
    return m_parentCell.asRect();
  }
};


template<typename NumericType>
inline Cell *Cell::track(uint32_t nObjectId, NumericType x, NumericType y) {
  if (contains(x, y)) {
    return this;
  }
  // This is unlikely path, so we can afford to lock mutex here
  Cell *pNewCell = m_pOwner->getCell(x, y);
  if (pNewCell) {
    assert(pNewCell->contains(x, y));
    std::lock_guard guard(*pNewCell->m_pMutex);
    assert(!pNewCell->m_objectsIds.has(nObjectId));
    pNewCell->m_objectsIds.push(nObjectId, false);
  }

  std::lock_guard guard(*m_pMutex);
  m_objectsIds.removeFirst(nObjectId);
  assert(!m_objectsIds.has(nObjectId));
  return pNewCell;
}


template<typename NumericType>
Grid::iterator Grid::range(NumericType x, NumericType y,
                           NumericType width, NumericType height) const
{
  assert(width > 0);
  assert(height > 0);
  NumericType x_end = x + width;
  NumericType y_end = y + height;

  // Check if parent cell and user's rect has any intersections
  if (x >= m_parentCell.right() || x_end <= m_parentCell.left() ||
      y >= m_parentCell.top() || y_end <= m_parentCell.bottom()) {
    return end();
  }

  x = std::max(x, static_cast<NumericType>(m_parentCell.left()));
  y = std::max(y, static_cast<NumericType>(m_parentCell.bottom()));
  if (!m_parentCell.contains(x, y)) {
    return end();
  }

  x_end = std::min(x_end, static_cast<NumericType>(m_parentCell.right() - 1));
  y_end = std::min(y_end, static_cast<NumericType>(m_parentCell.top() - 1));
  assert(m_parentCell.contains(x_end, y_end));

  const size_t nBegin = indexOf(x, y);
  const size_t nEnd   = indexOf(x_end, y_end);

  assert(nBegin <= nEnd);
  assert(nEnd % m_width >= nBegin % m_width);
  assert(nEnd / m_width >= nBegin / m_width);

  const size_t regionWidth  = nEnd % m_width - nBegin % m_width + 1;
  const size_t regionHeight = nEnd / m_width - nBegin / m_width + 1;

  return iterator(this, nBegin, regionWidth, regionHeight);
}

}  // namespace world
