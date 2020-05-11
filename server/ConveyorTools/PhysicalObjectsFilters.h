#include "ObjectsFilter.h"

#include <Geometry/Rectangle.h>
#include <Newton/PhysicalObject.h>

// This file contains a number of filters, that can be used to select physical
// objects (by it's position or signature or smth)

namespace tools {

class PysicalObjectsFilter : public BaseObjectFilter
{
public:
  PysicalObjectsFilter(size_t nReserved = 256)
  {
    m_filteredInstances.reserve(nReserved);
  }

  void attachToContainer(utils::ObjectsContainerPtr<newton::PhysicalObject> pContainer)
  {
    m_pObjectsContainer = pContainer;
  }

  void reset() override
  {
    m_filteredInstances.clear();
  }

  void proceed() override
  {
    std::vector<newton::PhysicalObject*> const& objects =
        m_pObjectsContainer->getObjects();

    std::array<newton::PhysicalObject*, 64> buffer;
    size_t nElementsInBuffer = 0;

    for (uint32_t nObjectId = yieldId();
         nObjectId < objects.size();
         nObjectId = yieldId()) {
      newton::PhysicalObject* pObj = objects[nObjectId];
      if (!pObj || !filter(pObj)) {
        continue;
      }

      buffer[nElementsInBuffer++] = pObj;
      if (nElementsInBuffer == buffer.size()) {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_filteredInstances.insert(m_filteredInstances.end(),
                                   buffer.begin(), buffer.end());
        nElementsInBuffer = 0;
      }
    }

    if (nElementsInBuffer) {
      std::lock_guard<std::mutex> guard(m_mutex);
      m_filteredInstances.insert(m_filteredInstances.end(),
                                 buffer.begin(),
                                 buffer.begin() + nElementsInBuffer);
    }
  }

  // Return array of filtered objects
  std::vector<newton::PhysicalObject*> const& getFiltered() const {
    return m_filteredInstances;
  }

protected:
  virtual bool filter(newton::PhysicalObject const* pObj) = 0;

private:
  std::mutex m_mutex;
  std::vector<newton::PhysicalObject*>               m_filteredInstances;
  utils::ObjectsContainerPtr<newton::PhysicalObject> m_pObjectsContainer;
};


// This filter checks, that some physical object is at least partially
// (or fully) covered by some rectangle area
class RectangeFilter : public PysicalObjectsFilter
{
public:
  RectangeFilter() = default;
  RectangeFilter(geometry::Rectangle const& rectangle)
    : m_rectangle(rectangle)
  {}

  void setPosition(geometry::Rectangle const& rectangle)
  {
    m_rectangle = rectangle;
  }

protected:
  bool filter(newton::PhysicalObject const* pObj) override
  {
    return m_rectangle.isCoveredByCicle(pObj->getPosition(), pObj->getRadius());
  }

private:
  geometry::Rectangle m_rectangle;
};

using RectangeFilterPtr = std::shared_ptr<RectangeFilter>;

} // namespace tools
