#pragma once

#include <memory>
#include <Utils/GlobalContainer.h>
#include <Newton/PhysicalObject.h>
#include <Utils/Spinlock.h>
#include <Utils/YamlForwardDeclarations.h>
#include <World/Resources.h>

namespace world {

struct AsteroidComposition
{
  AsteroidComposition(double utility, double nSilicates, double nMetals, double nIce);
  AsteroidComposition() : AsteroidComposition(0.1, 1, 1, 1)
  {}

  double silicates_percent() const { return percents[Resource::Type::eSilicate]; }
  double metals_percent()    const { return percents[Resource::Type::eMetal]; }
  double ice_percent()       const { return percents[Resource::Type::eIce]; }

  void normalize(double utility = 1.0);

  double percents[Resource::Type::eTotalResources];
};

class Asteroid;
using AsteroidsContainer = utils::GlobalContainer<Asteroid>;

class Asteroid : public newton::PhysicalObject, public AsteroidsContainer
{
public:
  Asteroid();
  Asteroid(double radius, double weight, AsteroidComposition composition);

  bool loadState(YAML::Node const& data);

  AsteroidComposition const& getComposition() const { return m_composition; }

  uint32_t getAsteroidId() const {
    return utils::GlobalContainer<Asteroid>::getInstanceId();
  }

  double yield(Resource::Type eType, double amount);

private:
  AsteroidComposition m_composition;

  utils::Spinlock m_spinlock;
};

using AsteroidUptr = std::unique_ptr<Asteroid>;

} // namespace world
