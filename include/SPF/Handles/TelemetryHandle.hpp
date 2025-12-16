#pragma once

#include "SPF/Handles/IHandle.hpp"
#include <string>

SPF_NS_BEGIN
namespace Handles {
class TelemetryHandle : public IHandle {
 public:
  const std::string pluginName;

  TelemetryHandle(const std::string& pluginName) : pluginName(pluginName) {}
};
}  // namespace Handles
SPF_NS_END
