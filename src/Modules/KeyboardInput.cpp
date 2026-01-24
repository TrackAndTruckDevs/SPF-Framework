#include "SPF/Modules/KeyboardInput.hpp"
#include "SPF/System/VirtualKeyMapping.hpp"
#include "SPF/Input/InputEvents.hpp"

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

nlohmann::ordered_json KeyboardInput::ToJson() const {
  return {{"type", "keyboard"},
          {"key", std::string(VirtualKeyMapping::GetInstance().GetKeyName(m_key))}};
}

std::string KeyboardInput::GetDisplayName() const { return VirtualKeyMapping::GetInstance().GetKeyDisplayName(m_key); }

bool KeyboardInput::IsValid() const { return m_key != System::Keyboard::Unknown; }

InputType KeyboardInput::GetType() const { return InputType::Keyboard; }

bool KeyboardInput::IsSameAs(const IBindableInput& other) const {
  if (other.GetType() != InputType::Keyboard) {
    return false;
  }

  const auto& otherKeyboardInput = static_cast<const KeyboardInput&>(other);
  return this->m_key == otherKeyboardInput.m_key;
}

}  // namespace Modules
SPF_NS_END