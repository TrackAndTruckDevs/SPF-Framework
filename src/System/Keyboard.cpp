#include "SPF/System/Keyboard.hpp"

#include "SPF/System/VirtualKeyMapping.hpp"

#include <string>

namespace SPF::System {
std::string ToString(Keyboard key) { return VirtualKeyMapping::GetInstance().GetKeyName(key); }

Keyboard FromString(const std::string& str) { return VirtualKeyMapping::GetInstance().GetKey(str); }
}  // namespace SPF::System
