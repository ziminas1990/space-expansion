#include <gtest/gtest.h>

#include <World/CelestialBodies/Asteroid.h>
#include <World/Resources.h>

namespace world {

namespace {

ResourcesArray traceMetalComposition()
{
  return ResourcesArray()
      .metals(0.01)
      .silicates(0.5)
      .ice(0.5)
      .stones(15.0);
}

double totalMass(ResourcesArray const& resources)
{
  double mass = 0;
  for (Resource::Type eType : Resource::MaterialResources) {
    mass += resources[eType];
  }
  return mass;
}

} // namespace

TEST(AsteroidYieldTests, LargeChunkStaysWithinStock)
{
  ASSERT_TRUE(Resource::initialize());

  // 1. mine trace-metal asteroids in civilian-miner sized chunks
  for (uint32_t seed = 1; seed <= 40; ++seed) {
    // 1.1 build an asteroid with a trace of metal
    Asteroid asteroid(5.0, traceMetalComposition(), seed);

    // 1.2 yield 1000 kg cycles and check each chunk fits the stock
    for (int cycle = 0; cycle < 30 && asteroid.getWeight() >= 1.0; ++cycle) {
      const double massBefore = asteroid.getWeight();
      const ResourcesArray composition = asteroid.getComposition();
      const ResourcesArray mined = asteroid.yield(1000.0);

      double taken = 0;
      for (Resource::Type eType : Resource::MaterialResources) {
        const double stock = massBefore * composition[eType];
        EXPECT_GE(mined[eType], 0.0) << seed;
        EXPECT_LE(mined[eType], stock) << seed;
        taken += mined[eType];
      }
      EXPECT_GT(taken, 0.0) << seed;
      EXPECT_LE(taken, 1000.0 + 1e-3) << seed;
      EXPECT_NEAR(asteroid.getWeight(), massBefore - taken, 1e-2) << seed;
    }
  }
}

TEST(AsteroidYieldTests, DepletingScoopStaysWithinStock)
{
  ASSERT_TRUE(Resource::initialize());

  // 1. empty trace-metal asteroids in one scoop
  for (uint32_t seed = 1; seed <= 80; ++seed) {
    // 1.1 build an asteroid with a trace of metal
    Asteroid asteroid(5.0, traceMetalComposition(), seed);
    const double massBefore = asteroid.getWeight();
    const ResourcesArray composition = asteroid.getComposition();

    // 1.2 scoop the whole mass and check the yield fits the stock
    const ResourcesArray mined = asteroid.yield(massBefore);
    for (Resource::Type eType : Resource::MaterialResources) {
      const double stock = massBefore * composition[eType];
      EXPECT_GE(mined[eType], 0.0) << seed;
      EXPECT_LE(mined[eType], stock) << seed;
    }
    EXPECT_NEAR(totalMass(mined), massBefore, 1e-2) << seed;
    EXPECT_LT(asteroid.getWeight(), 1.0) << seed;
    EXPECT_GT(asteroid.getRadius(), 0.0) << seed;
  }
}

} // namespace world
