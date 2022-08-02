#include <AdministratorPanel/BasicManipulator.h>

#include <Utils/ItemsConverter.h>
#include <World/CelestialBodies/Asteroid.h>
#include <Modules/Ship/Ship.h>

namespace administrator {

static newton::PhysicalObject* getObject(
  const admin::BasicManipulator::ObjectId& objectId)
{
  using AsteroidsContainer = utils::GlobalContainer<world::Asteroid>;
  using ShipsContainer     = utils::GlobalContainer<modules::Ship>;

  switch (objectId.object_type()) {
    case spex::ObjectType::OBJECT_ASTEROID: {
      if (objectId.id() < AsteroidsContainer::Size()) {
        return AsteroidsContainer::Instance(objectId.id());
      }
      return nullptr;
    }
    case spex::ObjectType::OBJECT_SHIP: {
      if (objectId.id() < ShipsContainer::Size()) {
        return ShipsContainer::Instance(objectId.id());
      }
      return nullptr;
    }
    default: {
      return nullptr;
    }
  }
}

void BasicManipulator::handleMessage(uint32_t nSessionId, 
                                     const admin::BasicManipulator& message)
{
  switch (message.choice_case()) {
    case admin::BasicManipulator::kObjectReq: {
      onObjectRequest(nSessionId, message.object_req());
      return;
    }
    case admin::BasicManipulator::kMove: {
      moveObject(nSessionId, message.move());
      return;
    }
    default: {
      return;
    }
  }
}

void BasicManipulator::onObjectRequest(
      uint32_t nSessionId,
      const admin::BasicManipulator::ObjectId& objectId)
{
  const newton::PhysicalObject* pObject = getObject(objectId);
  if (pObject) {
    sendObject(nSessionId, pObject);
    return;
  }
  sendProblem(nSessionId, admin::BasicManipulator::OBJECT_DOESNT_EXIST);
}

void BasicManipulator::moveObject(uint32_t nSessionId,
                                  const admin::BasicManipulator::Move& request)
{
  newton::PhysicalObject* pObject = getObject(request.object_id());
  if (pObject) {
    geometry::Point position;
    geometry::Vector velocity;
    utils::convert(request.position(), &position, &velocity);
    pObject->moveTo(position);
    pObject->setVelocity(velocity);
    sendMovedAt(nSessionId);
    return;
  }
  sendProblem(nSessionId, admin::BasicManipulator::OBJECT_DOESNT_EXIST);
}

bool BasicManipulator::sendProblem(uint32_t nSessionId,
                                   admin::BasicManipulator::Status problem)
{
  admin::Message message;
  message.mutable_manipulator()->set_problem(problem);
  return m_pChannel && m_pChannel->send(nSessionId, std::move(message));
}

bool BasicManipulator::sendObject(uint32_t nSessionId, 
                                  const newton::PhysicalObject* pObject)
{
  admin::Message response;
  spex::PhysicalObject* body = response.mutable_manipulator()->mutable_object();
  utils::convert(pObject, body);
  return m_pChannel && m_pChannel->send(nSessionId, std::move(response));
}

bool BasicManipulator::sendMovedAt(uint32_t nSessionId)
{
  admin::Message response;
  response.mutable_manipulator()->set_moved_at(utils::GlobalClock::now());
  return m_pChannel && m_pChannel->send(nSessionId, std::move(response));
}

} // namespace administrator