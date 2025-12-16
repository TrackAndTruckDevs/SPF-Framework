#pragma once

#include "IHandle.hpp"
#include <string>

SPF_NS_BEGIN
namespace Handles {
struct KeyBindsHandle : public IHandle {
  const std::string pluginName;

  KeyBindsHandle(const std::string& name) : pluginName(name) {}
};
}  // namespace Handles
SPF_NS_END
