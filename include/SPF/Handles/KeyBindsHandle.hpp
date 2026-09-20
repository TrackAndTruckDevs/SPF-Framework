#pragma once

#include "IHandle.hpp"

#include <string>

namespace SPF::Handles {
struct KeyBindsHandle : public IHandle {
  const std::string pluginName;

  KeyBindsHandle(const std::string& name) : pluginName(name) {}
};
}  // namespace SPF::Handles
