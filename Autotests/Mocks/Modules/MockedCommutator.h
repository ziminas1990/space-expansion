#pragma once

#include <memory>
#include <Protocol.pb.h>
#include <Autotests/TestUtils/ProtobufSyncPipe.h>
#include <Autotests/Mocks/MockedBaseModule.h>
#include <Modules/BaseModule.h>

namespace autotests {

struct ModuleInfo
{
  uint32_t    nSlotId;
  std::string sModuleType;
};

using ModulesList = std::vector<ModuleInfo>;

class ClientCommutator
{
public:
  ClientCommutator(uint32_t nTunnelId) : m_nTunnelId(nTunnelId) {}

  void attachToSyncChannel(ProtobufSyncPipePtr pSyncPipe)
  { m_pSyncPipe = pSyncPipe; }

  // Sending GetTotalSlots request and checking response
  bool getTotalSlots(uint32_t nExpectedSlots);

  bool getAttachedModulesList(uint32_t nTotal, ModulesList& attachedModules);

  bool openTunnel(uint32_t nSlotId, bool lExpectSuccess = true,
                  uint32_t *pOpenedTunnelId = nullptr);
  bool sendOpenTunnel(uint32_t nSlotId);
  bool waitOpenTunnelSuccess(uint32_t *pOpenedTunnelId = nullptr);
  bool waitOpenTunnelFailed();

private:
  uint32_t            m_nTunnelId;
  ProtobufSyncPipePtr m_pSyncPipe;
};


class MockedCommutator : public MockedBaseModule
{
public:
  MockedCommutator();

  bool waitOpenTunnel(uint32_t nSessionId, uint32_t nSlotId);
  bool sendOpenTunnelSuccess(uint32_t nSessionId, uint32_t nTunnelId);
  bool sendOpenTunnelFailed(uint32_t nSessionId);
};


using ClientCommutatorPtr = std::shared_ptr<ClientCommutator>;
using MockedCommutatorPtr = std::shared_ptr<MockedCommutator>;

} // namespace autotests
