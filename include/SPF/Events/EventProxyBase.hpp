#pragma once

namespace SPF::Events {
class EventManager;  // Forward declaration

class EventProxyBase {
 protected:
  EventManager& m_eventManager;

 public:
  EventProxyBase(EventManager& eventManager) : m_eventManager(eventManager) {}
  virtual ~EventProxyBase() = default;
};
}  // namespace SPF::Events
