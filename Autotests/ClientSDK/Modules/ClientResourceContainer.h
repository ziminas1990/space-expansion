#pragma once

#include <Autotests/ClientSDK/Interfaces.h>
#include <Autotests/ClientSDK/ClientBaseModule.h>
#include <World/Resources.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <stdint.h>
#include <vector>

namespace autotests { namespace client {

class ResourceContainer : public ClientBaseModule
{
public:

  struct Content {
    Content() :
      m_nVolume(0), m_nUsedSpace(0.0), m_amount(world::Resource::eTotalResources)
    {}

    uint32_t m_nVolume;
    double   m_nUsedSpace;
    std::vector<double> m_amount;
  };


  bool getContent(Content& content);

};

using ResourceContainerPtr = std::shared_ptr<ResourceContainer>;

}}  // namespace autotests::client
