#include <World/Grid.h>
#include <gtest/gtest.h>
#include <Utils/Randomizer.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <boost/fiber/barrier.hpp>

namespace world {

TEST(GridTests, CellContainsTest) {
  const int32_t left   = -20;
  const int32_t bottom = -10;
  const int32_t width  = 60;

  Cell cell(nullptr, left, bottom, width);
  ASSERT_EQ(left,           cell.left());
  ASSERT_EQ(left + width,   cell.right());
  ASSERT_EQ(bottom,         cell.bottom());
  ASSERT_EQ(bottom + width, cell.top());

  for (int32_t x = left; x < left + width; ++x) {
    for (int32_t y = bottom; y < bottom + width; ++y) {
      ASSERT_TRUE(cell.contains(x, y));
    }
    ASSERT_FALSE(cell.contains(x, bottom + width));
  }

  for (int32_t y = left; y < bottom + width; ++y) {
    ASSERT_FALSE(cell.contains(left + width, y));
  }

  int32_t centerX = left + width / 2;
  int32_t centerY = bottom + width / 2;

  ASSERT_FALSE(cell.contains(centerX,         centerY + width));
  ASSERT_FALSE(cell.contains(centerX,         centerY - width));
  ASSERT_FALSE(cell.contains(centerX + width, centerY));
  ASSERT_FALSE(cell.contains(centerX - width, centerY));
  ASSERT_FALSE(cell.contains(centerX + width, centerY + width));
  ASSERT_FALSE(cell.contains(centerX + width, centerY - width));
  ASSERT_FALSE(cell.contains(centerX - width, centerY + width));
  ASSERT_FALSE(cell.contains(centerX - width, centerY - width));
}


struct Point {
  uint32_t         m_id;
  geometry::Point  m_position;
  geometry::Vector m_velocity;
  Cell*            m_pCell;

  double x() const { return m_position.x; }
  double y() const { return m_position.y; }
};

Point createPoint(uint32_t id, Grid& grid, double velocityMps)
{
  double x = utils::Randomizer::yeild<double>(grid.left(),   grid.right());
  double y = utils::Randomizer::yeild<double>(grid.left(),   grid.right());
  geometry::Vector velocity;
  utils::Randomizer::yeild(velocity, velocityMps);
  if (velocity.getLength() < velocityMps / 10) {
    velocity.setLength(velocityMps / 10);
  }
  Cell* pCell = grid.getCell(x, y);
  pCell->add(id, x, y);
  return Point{id, geometry::Point(x, y), velocity, pCell};
}

TEST(GridTests, TrackSinglePoint) {
  for (uint32_t i = 0; i < 100; ++i) {
    Grid grid(0xFF, 250);

    utils::Randomizer::setPattern(i);
    Point point = createPoint(i, grid, 10);

    while (point.m_pCell) {
      point.m_position += point.m_velocity;
      point.m_pCell = point.m_pCell->track(point.m_id, point.x(), point.y());
      if (point.m_pCell) {
        ASSERT_TRUE(point.m_pCell->contains(point.x(), point.y()))
            << "Case #" << i;
      }
    }
    // Point left the grid
    ASSERT_FALSE(grid.contains(point.x(), point.y())) << "Case #" << i;
  }
}

TEST(GridTests, TrackMultiplePoints) {
  for (uint32_t i = 0; i < 10; ++i) {
    utils::Randomizer::setPattern(i);
    Grid grid(64, 500);

    std::vector<Point> points;
    points.reserve(1000);
    for (size_t i = 0; i < 500; ++i) {
      points.push_back(createPoint(i, grid, 250));
    }

    bool hasObjectsInbound = true;
    while (hasObjectsInbound) {
      // Move all points and see which are still inbound
      std::set<uint32_t> objectsInbound;
      for (Point& point: points) {
        point.m_position += point.m_velocity;
        if (point.m_pCell) {
          point.m_pCell = point.m_pCell->track(point.m_id, point.x(), point.y());
          if (point.m_pCell) {
            objectsInbound.insert(point.m_id);
          }
        }
      }
      hasObjectsInbound = objectsInbound.size() > 0;

      // Check all inbound objects
      while (!objectsInbound.empty()) {
        // Get some random point
        const uint32_t objectId  = *objectsInbound.begin();
        const Point&   point     = points[objectId];
        // Check that all neighbors have the same cell
        const auto&    neighbors = point.m_pCell->getObjects();
        for (uint32_t neighborId: neighbors.data()) {
          const Point& neighbor = points[neighborId];
          ASSERT_EQ(neighbor.m_pCell, point.m_pCell);
          ASSERT_TRUE(point.m_pCell->contains(neighbor.x(), neighbor.y()));
          objectsInbound.erase(neighborId);
        }
      }
    }

    // Check that all points are not inbnound
    for (const Point& point: points) {
      ASSERT_FALSE(grid.contains(point.x(), point.y()));
    }

    // Check that there are no cells with any object
    for (const Cell& cell: grid.cells()) {
      ASSERT_EQ(0, cell.getObjects().size());
    }
  }
}

TEST(GridTests, TrackMultiplePointsMultiThreaded) {
  const size_t totalThread = 6;
  const size_t totalPoints = 1000;

  for (uint32_t gridWidth: {1, 2, 4, 8, 16, 32}) {
    utils::Randomizer::setPattern(gridWidth);
    Grid grid(gridWidth, 500);

    std::vector<Point> points;
    points.reserve(totalPoints);
    for (size_t i = 0; i < totalPoints; ++i) {
      points.push_back(createPoint(i, grid, 100));
    }

    boost::fibers::barrier barrier(totalThread + 1);
    std::atomic_size_t     idx;
    bool                   continueFlag = true;

    auto worker = [&barrier, &points, &idx, &continueFlag] () {
      const size_t batch = 16;
      while (true) {
        barrier.wait();
        if (!continueFlag) {
          return;
        }
        size_t begin = idx.fetch_add(batch);
        for (; begin < points.size(); begin = idx.fetch_add(batch)) {
          size_t end = std::min(begin + batch, points.size());
          for (size_t objectId = begin; objectId < end; ++objectId) {
            Point& point = points[objectId];
            point.m_position += point.m_velocity;
            if (point.m_pCell) {
              point.m_pCell = point.m_pCell->track(
                    point.m_id, point.x(), point.y());
            }
          }
        }
        barrier.wait();
      }
    };

    std::vector<std::thread> workers;
    for (size_t i = 0; i < totalThread; ++i) {
      workers.emplace_back(worker);
    }

    size_t iteration = 0;
    while (continueFlag) {
      // Move all points and see which are still inbound
      idx.store(0);
      barrier.wait();
      // Workers are moving objects here
      barrier.wait();

      std::set<uint32_t> objectsInbound;
      for (const Point& point: points) {
        if (point.m_pCell) {
          objectsInbound.insert(point.m_id);
        }
      }
      continueFlag = objectsInbound.size() > 0;
      ++iteration;

      // Check all inbound objects
      while (!objectsInbound.empty()) {
        // Get some random point
        const uint32_t objectId  = *objectsInbound.begin();
        const Point&   point     = points[objectId];
        // Check that all neighbors have the same cell
        const auto&    neighbors = point.m_pCell->getObjects();
        for (uint32_t neighborId: neighbors.data()) {
          const Point& neighbor = points[neighborId];
          ASSERT_EQ(neighbor.m_pCell, point.m_pCell);
          ASSERT_TRUE(point.m_pCell->contains(neighbor.x(), neighbor.y()));
          objectsInbound.erase(neighborId);
        }
      }
    }

    // Release workers (they are going to quit)
    barrier.wait();
    for (auto& worker: workers) {
      worker.join();
    }

    // Check that all points are not inbnound
    for (const Point& point: points) {
      ASSERT_FALSE(grid.contains(point.x(), point.y()));
    }

    // Check that there are no cells with any object
    for (const Cell& cell: grid.cells()) {
      ASSERT_EQ(0, cell.getObjects().size());
    }
  }
}

}  // namespace world
