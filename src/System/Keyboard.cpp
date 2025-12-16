#include "SPF/System/Keyboard.hpp"
#include "SPF/System/VirtualKeyMapping.hpp"

SPF_NS_BEGIN
namespace System {
std::string ToString(Keyboard key) { return VirtualKeyMapping::GetInstance().GetKeyName(key); }

Keyboard FromString(const std::string& str) { return VirtualKeyMapping::GetInstance().GetKey(str); }
}  // namespace System
SPF_NS_END
