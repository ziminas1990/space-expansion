#include "Screen.h"

#include <ConveyorTools/ObjectsFilter.h>
#include <ConveyorTools/PhysicalObjectsFilters.h>
#include <World/ObjectTypes.h>
#include <World/ObjectContainers.h>
#include <Utils/ItemsConverter.h>
#include <SystemManager.h>

namespace modules { class Ship; }
namespace world { class Asteroid; }

namespace administrator {

Screen::Screen() : m_pFilter(std::make_shared<tools::RectangeFilter>())
{}

void Screen::setup(SystemManager *pSystemManager, network::IPrivilegedChannelPtr pChannel)
{
  m_pChannel = pChannel;

  m_pFilterManager = pSystemManager->getFilteringManager();
  m_pFilterManager->registerFilter(m_pFilter);
}

void Screen::proceed(uint32_t nIntervalUs)
{
  m_containersCache.proceed(nIntervalUs);
  if (m_nSessionId == network::gInvalidSessionId) {
    return;
  }
  if (m_pFilter->isWaitingForUpdate()) {
    return;
  }

  const size_t nObjectsPerMessage = 50;
  std::vector<newton::PhysicalObject*> const& filtered = m_pFilter->getFiltered();
  for (size_t begin = 0; begin < filtered.size(); begin += nObjectsPerMessage) {
    size_t end = std::min(begin + nObjectsPerMessage, filtered.size());

    admin::Message message;
    admin::Screen* pResponse = message.mutable_screen();

    spex::PhysicalObjectsList* pChunk = pResponse->mutable_objects();
    pChunk->set_left(static_cast<uint32_t>(filtered.size() - end));
    for(size_t i = begin; i < end; ++i) {
      if (filtered[i]) {
        utils::convert(filtered[i], pChunk->add_object());
      }
    }

    if (pChunk->object_size()) {
      m_pChannel->send(m_nSessionId, std::move(message));
    }
  }

  m_nSessionId = network::gInvalidSessionId;
}

void Screen::handleMessage(uint32_t nSessionId, admin::Screen const& message)
{
  switch (message.choice_case()) {
    case admin::Screen::kMove:
      move(nSessionId, message.move());
      return;
    case admin::Screen::kShow:
      show(nSessionId, message.show());
      return;
    default:
      assert("Unexpected command" == nullptr);
  }
}

void Screen::move(uint32_t nSessionId, admin::Screen::Position const& position)
{
   m_pFilter->setPosition(
         geometry::Rectangle(
           geometry::Point(position.x(), position.y()),
           position.width(),
           position.height()));
   sendStatus(nSessionId, admin::Screen::SUCCESS);
}

void Screen::show(uint32_t nSessionId, spex::ObjectType eType)
{
  if (nSessionId == network::gInvalidSessionId) {
    sendStatus(nSessionId, admin::Screen::FAILED);
    return;
  }

  world::PhysicalObjectsContainerPtr pObjects =
      m_containersCache.getContainerWith(utils::convert(eType));
  if (!pObjects) {
    sendStatus(nSessionId, admin::Screen::FAILED);
    return;
  }
  m_pFilter->attachToContainer(pObjects);
  m_pFilter->updateAsSoonAsPossible();
  m_nSessionId = nSessionId;
}

bool Screen::sendStatus(uint32_t nSessionId, admin::Screen::Status eStatus)
{
  admin::Message message;
  message.mutable_screen()->set_status(eStatus);
  return m_pChannel->send(nSessionId, std::move(message));
}

} // namespace administrator
