#include "SPF/Modules/GamepadInput.hpp"

#include "SPF/Input/InputEvents.hpp"
#include "SPF/Input/InputManager.hpp"
#include "SPF/Modules/IBindableInput.hpp"
#include "SPF/System/GamepadButton.hpp"
#include "SPF/System/GamepadButtonMapping.hpp"

#include "nlohmann/json_fwd.hpp"

#include <cstdint>
#include <map>
#include <set>
#include <string>

namespace SPF::Modules {
using namespace SPF::System;

GamepadInput::GamepadInput(const nlohmann::ordered_json& config) {
  std::string buttonName = config.value("key", config.value("button", "UNKNOWN_BUTTON"));
  m_button = GamepadButtonMapping::GetInstance().GetButton(buttonName);
  ReadDevice(config);
}

bool GamepadInput::IsTriggeredBy(const Input::GamepadEvent& event) const {
  // Check if the button matches
  return event.button == m_button && event.device == m_device;
}

nlohmann::ordered_json GamepadInput::ToJson() const {
  nlohmann::ordered_json j = {{"type", "gamepad"}, {"key", GamepadButtonMapping::GetInstance().GetButtonName(m_button)}};
  WriteDevice(j);
  return j;
}

std::string GamepadInput::GetDisplayName() const {
  // Get the globally detected device type from the InputManager
  System::DeviceType detectedType = Input::InputManager::GetInstance().GetDetectedGamepadType();

  // Get the correct display name based on the detected type
  return GamepadButtonMapping::GetInstance().GetButtonDisplayName(m_button, detectedType);
}

bool GamepadInput::IsValid() const { return m_button != System::GamepadButton::Unknown; }

InputType GamepadInput::GetType() const { return InputType::Gamepad; }

uint32_t GamepadInput::GetHardwareCode() const { return Input::WithDevice(0x02000000 | static_cast<uint32_t>(m_button), m_device); }

bool GamepadInput::IsActive(const std::set<uint32_t>& pressedCodes) const { return pressedCodes.count(GetHardwareCode()) > 0; }

bool GamepadInput::InvolvesHardwareCode(uint32_t code) const { return GetHardwareCode() == code; }

bool GamepadInput::IsSameAs(const IBindableInput& other) const {
  if (other.GetType() != InputType::Gamepad) {
    return false;
  }

  const auto& otherGamepadInput = static_cast<const GamepadInput&>(other);
  return this->m_button == otherGamepadInput.m_button && m_device == other.GetDevice();
}

float GamepadInput::GetValue(const std::set<uint32_t>& pressedHardwareCodes, const std::map<uint32_t, float>& axisValues) const { return IsActive(pressedHardwareCodes) ? 1.0f : 0.0f; }

}  // namespace SPF::Modules
