#pragma once

#include <stdint.h>
#include <unordered_map>
#include <memory>
#include <vector>
#include <assert.h>

#include <Utils/Mutex.h>
#include <Utils/UnorderedVector.h>

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

  int32_t left()   const { return m_x; }
  int32_t right()  const { return m_x + static_cast<int32_t>(m_width); }
  int32_t bottom() const { return m_y; }
  int32_t top()    const { return m_y + static_cast<int32_t>(m_width); }
};


class Grid {
private:
  uint32_t          m_cellWidth = 0;
  uint8_t           m_width     = 0;
  std::vector<Cell> m_cells;
  Cell              m_parentCell;

  template<typename NumericType>
  size_t indexOf(NumericType x, NumericType y) const {
    const int32_t       i      = (x - m_parentCell.left()) / m_cellWidth;
    const int32_t       j      = (y - m_parentCell.bottom()) / m_cellWidth;
    return static_cast<size_t>(j * m_width + i);
  }

public:
  Grid(uint8_t width, uint32_t cellWidth);

  template<typename NumericType>
  const Cell* getCell(NumericType x, NumericType y) const {
    return contains(x, y) ? &m_cells[indexOf(x, y)] : nullptr;
  }

  template<typename NumericType>
  Cell* getCell(NumericType x, NumericType y) {
    return contains(x, y) ? &m_cells[indexOf(x, y)] : nullptr;
  }

  const std::vector<Cell>& cells() const { return m_cells; }

  template<typename NumericType>
  bool contains(NumericType x, NumericType y) const {
    return m_parentCell.contains(x, y);
  }

  int32_t left()   const { return m_parentCell.left(); }
  int32_t right()  const { return m_parentCell.right(); }
  int32_t bottom() const { return m_parentCell.bottom(); }
  int32_t top()    const { return m_parentCell.top(); }
};


template<typename NumericType>
inline Cell *Cell::track(uint32_t nObjectId, NumericType x, NumericType y) {
  if (contains(x, y)) {
    return this;
  }
  // It should happen very seldom, so we can afford to have a mutex lock
  // in this case
  Cell *pNewCell = m_pOwner->getCell(x, y);
  if (pNewCell) {
    assert(pNewCell->contains(x, y));
    std::lock_guard guard(*pNewCell->m_pMutex);
    //assert(!pNewCell->m_objectsIds.has(nObjectId));
    pNewCell->m_objectsIds.push(nObjectId, false);
  }

  std::lock_guard guard(*m_pMutex);
  m_objectsIds.removeFirst(nObjectId);
  assert(!m_objectsIds.has(nObjectId));
  return pNewCell;
}


}  // namespace world
