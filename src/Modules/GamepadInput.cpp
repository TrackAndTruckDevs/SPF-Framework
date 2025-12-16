#include <Windows.h>  // Pre-include for safety

#include "SPF/Modules/GamepadInput.hpp"
#include "SPF/System/GamepadButtonMapping.hpp"
#include "SPF/Input/InputManager.hpp"  // Include the InputManager
#include "SPF/Input/InputEvents.hpp"

SPF_NS_BEGIN
namespace Modules {
using namespace SPF::System;

GamepadInput::GamepadInput(const nlohmann::json& config) {
  std::string buttonName = config.value("button", "UNKNOWN_BUTTON");
  m_button = GamepadButtonMapping::GetInstance().GetButton(buttonName);

  std::string pressTypeStr = config.value("press_type", "short");
  if (pressTypeStr == "long") {
    m_pressType = Input::PressType::Long;
  } else {
    m_pressType = Input::PressType::Short;
  }
}

bool GamepadInput::IsTriggeredBy(const Input::GamepadEvent& event) const {
  // Check if the button matches
  return event.button == m_button;
}

nlohmann::json GamepadInput::ToJson() const {
  return {{"type", "gamepad"},
          {"button", GamepadButtonMapping::GetInstance().GetButtonName(m_button)},
          {"press_type", (m_pressType == Input::PressType::Long ? "long" : "short")}};
}

std::string GamepadInput::GetDisplayName() const {
  // Get the globally detected device type from the InputManager
  System::DeviceType detectedType = Input::InputManager::GetInstance().GetDetectedGamepadType();

  // Get the correct display name based on the detected type
  return GamepadButtonMapping::GetInstance().GetButtonDisplayName(m_button, detectedType);
}

bool GamepadInput::IsValid() const { return m_button != System::GamepadButton::Unknown; }

InputType GamepadInput::GetType() const { return InputType::Gamepad; }

bool GamepadInput::IsSameAs(const IBindableInput& other) const {
  if (other.GetType() != InputType::Gamepad) {
    return false;
  }

  const auto& otherGamepadInput = static_cast<const GamepadInput&>(other);
  return this->m_button == otherGamepadInput.m_button && this->m_pressType == otherGamepadInput.m_pressType;
}

}  // namespace Modules
SPF_NS_END
