#pragma once

#include <stdint.h>
#include <Privileged.pb.h>
#include <Network/Interfaces.h>

namespace newton {
class PhysicalObject;
}

namespace administrator {

class BasicManipulator {
  network::IPrivilegedChannelPtr m_pChannel;
    // Channel to client

public:
  void handleMessage(uint32_t nSessionId, 
                     const admin::BasicManipulator& message);

private:
  void onObjectRequest(uint32_t nSessionId,
                       const admin::BasicManipulator::ObjectId& objectId);

  void moveObject(uint32_t nSessionId,
                  const admin::BasicManipulator::Move& request);

  bool sendProblem(uint32_t nSessionId,
                   admin::BasicManipulator::Status problem);
  bool sendObject(uint32_t nSessionId, const newton::PhysicalObject* pObject);
  bool sendMovedAt(uint32_t nSessionId);

};

} // namespace administrator
