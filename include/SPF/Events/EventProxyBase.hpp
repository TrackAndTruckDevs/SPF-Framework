#pragma once

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN

namespace Events {
class EventManager;  // Forward declaration

class EventProxyBase {
 protected:
  EventManager& m_eventManager;

 public:
  EventProxyBase(EventManager& eventManager) : m_eventManager(eventManager) {}
  virtual ~EventProxyBase() = default;
};
}  // namespace Events

SPF_NS_END
