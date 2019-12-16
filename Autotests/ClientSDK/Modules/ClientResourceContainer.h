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
  enum Status {
    eStatusOk,
    eStatusError,

    ePortAlreadyOpen,
    ePortIsNotOpened,
    ePortHasBeenClosed,
    eInvalidAccessKey,

    eTransferGotInvalidReport,
    eTransferIncomplete,  // session closed, but according to reports, not all resources
                          // had been transffered
    eTransferTooFar,
    eTransferInProgress,
    eNotEnoughResources,
    eTransferError
  };

  struct Content {
    Content() :
      m_nVolume(0), m_nUsedSpace(0.0), m_amount(world::Resources::eTotalResources)
    {}

    double mettals()   const { return m_amount[world::Resources::eMettal]; }
    double silicates() const { return m_amount[world::Resources::eSilicate]; }
    double ice()       const { return m_amount[world::Resources::eIce]; }

    Content& setMettals(double amount);
    Content& addMettals(double amount) { return setMettals(mettals() + amount); }

    Content& setSilicates(double amount);
    Content& addSilicates(double amount) { return setSilicates(silicates() + amount); }

    Content& setIce(double amount);
    Content& addIce(double amount) { return setIce(ice() + amount); }

    Content& set(world::Resources::Type eType, double amount);

    uint32_t            m_nVolume;
    double              m_nUsedSpace;
    std::vector<double> m_amount;
  };

  bool getContent(Content& content);

  Status openPort(uint32_t nAccessKey, uint32_t& nPortId);
  Status closePort();
  Status transfer(uint32_t nPortId, uint32_t nAccessKey,
                  world::Resources::Type type, double amount);

  bool checkContent(Content const& expected);
};

using ResourceContainerPtr = std::shared_ptr<ResourceContainer>;

inline
ResourceContainer::Content& ResourceContainer::Content::setMettals(double amount)
{
  return set(world::Resources::eMettal, amount);
}

inline
ResourceContainer::Content &ResourceContainer::Content::setSilicates(double amount)
{
  return set(world::Resources::eSilicate, amount);
}

inline
ResourceContainer::Content& ResourceContainer::Content::setIce(double amount)
{
  return set(world::Resources::eIce, amount);
}

}}  // namespace autotests::client
