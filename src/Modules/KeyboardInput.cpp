#include "SPF/Modules/KeyboardInput.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Input/InputEvents.hpp"
#include "SPF/Modules/IBindableInput.hpp"
#include "SPF/System/Keyboard.hpp"
#include "SPF/System/VirtualKeyMapping.hpp"

#include "nlohmann/json_fwd.hpp"

#include <cstdint>
#include <map>
#include <set>
#include <string>

SPF_NS_BEGIN
namespace Modules {
using namespace SPF::System;

KeyboardInput::KeyboardInput(const nlohmann::ordered_json& config) {
  std::string keyName = config.value("key", "Unknown");
  m_key = VirtualKeyMapping::GetInstance().GetKey(keyName);
}

bool KeyboardInput::IsTriggeredBy(const Input::KeyboardEvent& event) const {
  // Check if the key matches
  return event.key == m_key;
}

bool KeyboardInput::IsTriggeredBy(System::Keyboard key) const { return key == m_key; }

nlohmann::ordered_json KeyboardInput::ToJson() const { return {{"type", "keyboard"}, {"key", std::string(VirtualKeyMapping::GetInstance().GetKeyName(m_key))}}; }

std::string KeyboardInput::GetDisplayName() const { return VirtualKeyMapping::GetInstance().GetKeyDisplayName(m_key); }

bool KeyboardInput::IsValid() const { return m_key != System::Keyboard::Unknown; }

InputType KeyboardInput::GetType() const { return InputType::Keyboard; }

uint32_t KeyboardInput::GetHardwareCode() const { return 0x01000000 | static_cast<uint32_t>(m_key); }

bool KeyboardInput::IsActive(const std::set<uint32_t>& pressedCodes) const { return pressedCodes.count(GetHardwareCode()) > 0; }

bool KeyboardInput::InvolvesHardwareCode(uint32_t code) const { return GetHardwareCode() == code; }

bool KeyboardInput::IsSameAs(const IBindableInput& other) const {
  if (other.GetType() != InputType::Keyboard) {
    return false;
  }

  const auto& otherKeyboardInput = static_cast<const KeyboardInput&>(other);
  return this->m_key == otherKeyboardInput.m_key;
}

float KeyboardInput::GetValue(const std::set<uint32_t>& pressedHardwareCodes, const std::map<uint32_t, float>& axisValues) const { return IsActive(pressedHardwareCodes) ? 1.0f : 0.0f; }

}  // namespace Modules
SPF_NS_END