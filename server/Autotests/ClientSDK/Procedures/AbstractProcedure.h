#pragma once

#include <memory>
#include <Autotests/ClientSDK/Interfaces.h>

namespace autotests { namespace client {

// Subclassng this class you MUST:
// 1. override prephare() function
// 2. override IProceedable::proceed() function
class AbstractProcedure : public IProceedable
{
public:
  enum State {
    eInactive,
    eActive,
    eFinished,
    eFailed,
    eInterrupted
  };

  virtual ~AbstractProcedure() = default;

  bool run() {
    if (m_eState != eInactive || !prephare())
      return false;
    m_eState = eActive;
    return true;
  }
  bool isComplete() const { return m_eState > eActive; }
  bool isSucceed()  const { return m_eState == eFinished; }

  void interrupt() {
    onInterrupted();
    m_eState = eInterrupted;
  }

protected:
  void finished()  { m_eState = eFinished; }
  void failed()    { m_eState = eFailed;   }

  virtual bool prephare()      { return true; }
  virtual void onInterrupted() {}

private:
  State m_eState = eInactive;
};

using AbstractProcedurePtr = std::shared_ptr<AbstractProcedure>;

}}  // namespace autotests::client
