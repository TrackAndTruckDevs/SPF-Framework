#include "SPF/Modules/MouseInput.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Input/InputEvents.hpp"
#include "SPF/Modules/IBindableInput.hpp"
#include "SPF/System/MouseButtonMapping.hpp"

#include "nlohmann/json_fwd.hpp"

#include <cstdint>
#include <map>
#include <set>
#include <string>

SPF_NS_BEGIN
namespace Modules {

MouseInput::MouseInput(const nlohmann::ordered_json& config) : m_button(System::MouseButton::Unknown) {
  if (config.contains("key") && config["key"].is_string()) {
    m_button = System::MouseButtonMapping::GetInstance().FromString(config["key"].get<std::string>());
  }
}

bool MouseInput::IsTriggeredBy(const Input::MouseButtonEvent& event) const {
  // Check if the event's button ID and press type matches our button and press type.
  // Note: The MouseInput object itself doesn't store pressType. The comparison here
  // is simply checking if the raw event matches the button. PressType matching
  // is handled by KeyBindsManager.
  return static_cast<int>(m_button) == event.iButton;
}

nlohmann::ordered_json MouseInput::ToJson() const { return {{"type", "mouse"}, {"key", System::MouseButtonMapping::GetInstance().ToString(m_button)}}; }

std::string MouseInput::GetDisplayName() const { return System::MouseButtonMapping::GetInstance().GetButtonDisplayName(m_button); }

bool MouseInput::IsValid() const {
  // A binding is considered invalid if the button is unknown OR if it's the reserved left mouse button.
  return m_button != System::MouseButton::Unknown && m_button != System::MouseButton::Left;
}

uint32_t MouseInput::GetHardwareCode() const { return 0x03000000 | static_cast<uint32_t>(m_button); }

bool MouseInput::IsActive(const std::set<uint32_t>& pressedCodes) const { return pressedCodes.count(GetHardwareCode()) > 0; }

bool MouseInput::InvolvesHardwareCode(uint32_t code) const { return GetHardwareCode() == code; }

bool MouseInput::IsSameAs(const IBindableInput& other) const {  // Check if the other object is also a MouseInput
  if (other.GetType() != InputType::Mouse) {
    return false;
  }

  // If so, cast it and compare the underlying button
  const auto& otherMouseInput = static_cast<const MouseInput&>(other);
  return this->m_button == otherMouseInput.m_button;
}

float MouseInput::GetValue(const std::set<uint32_t>& pressedHardwareCodes, const std::map<uint32_t, float>& axisValues) const { return IsActive(pressedHardwareCodes) ? 1.0f : 0.0f; }

}  // namespace Modules
SPF_NS_END
