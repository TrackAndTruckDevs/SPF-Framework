#pragma once

#include "SPF/Handles/IHandle.hpp"
#include "SPF/Modules/API/TelemetryApi.hpp" // Include for BaseSubscriptionHandler
#include <string>
#include <vector>
#include <memory> // For std::unique_ptr

SPF_NS_BEGIN
namespace Handles {
class TelemetryHandle : public IHandle {
 public:
  const std::string pluginName;

  TelemetryHandle(const std::string& pluginName) : pluginName(pluginName) {}

  // This vector will hold all active telemetry subscriptions for this plugin.
  // Using unique_ptr to BaseSubscriptionHandler allows polymorphism and RAII.
  std::vector<std::unique_ptr<Modules::API::TelemetryApi::BaseSubscriptionHandler>> m_subscriptionHandlers;
};
}  // namespace Handles
SPF_NS_END
