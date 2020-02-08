#include "Resources.h"


namespace world {

std::vector<double> Resource::density;

bool Resource::initialize()
{
  density.resize(eTotalResources);
  density[eMettal]   = 4500;  // Ti
  density[eIce]      = 916;
  density[eSilicate] = 2330;  // Si
  return true;
}

} // namespace world
