#include "SPF/Modules/GamepadAxisInput.hpp"
#include "SPF/System/GamepadButtonMapping.hpp"
#include "SPF/Input/InputManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include <algorithm>
#include <cmath>

SPF_NS_BEGIN
namespace Modules {
using namespace SPF::System;

GamepadAxisInput::GamepadAxisInput(const nlohmann::ordered_json& config) {
    m_axis = GamepadButtonMapping::GetInstance().GetButton(config.value("key", "UNKNOWN"));
    m_mode = config.value("mode", "analog");
    
    // Systemic Trigger Handling: Triggers are always 0..1 and don't need 'side' splitting
    std::string keyNameRaw = config.value("key", "");
    bool isTrigger = (keyNameRaw.find("TRIGGER") != std::string::npos);

    m_deadzone = config.value("deadzone", 0.0f);
    m_saturation = config.value("saturation", 1.0f);
    m_sensitivity = config.value("sensitivity", 1.0f);
    m_threshold = config.value("threshold", 0.5f);
    m_invert = config.value("invert", false);
    m_curve = config.value("curve", "linear");
    m_side = isTrigger ? "both" : config.value("side", "both");
    m_smoothing = config.value("smoothing", 0.0f);
    m_rangeMin = config.value("range_min", isTrigger ? 0.0f : -1.0f);
    m_rangeMax = config.value("range_max", 1.0f);
}

bool GamepadAxisInput::IsTriggeredBy(const Input::GamepadEvent& event) const {
    // For axes, we don't trigger actions directly via events in analog mode,
    // but the digital state machine uses GetValue().
    return false; 
}

uint32_t GamepadAxisInput::GetHardwareCode() const {
    // Using the 0xTT0100XX scheme for axes
    // We need a mapping from GamepadButton enum to axis index
    int index = 0;
    switch (m_axis) {
        case GamepadButton::LeftStickX: index = 0; break;
        case GamepadButton::LeftStickY: index = 1; break;
        case GamepadButton::RightStickX: index = 2; break;
        case GamepadButton::RightStickY: index = 3; break;
        case GamepadButton::LeftTriggerAxis: index = 4; break;
        case GamepadButton::RightTriggerAxis: index = 5; break;
        default: index = 0xFF; break;
    }
    return 0x02010000 | static_cast<uint32_t>(index);
}

bool GamepadAxisInput::IsActive(const std::set<uint32_t>& pressedCodes) const {
    // For axes, "Active" means past the threshold if in digital mode
    // or simply has a non-zero value. For simplicity in the existing manager:
    return pressedCodes.count(GetHardwareCode()) > 0;
}

float GamepadAxisInput::GetValue(const std::set<uint32_t>& pressedHardwareCodes, const std::map<uint32_t, float>& axisValues) const {
    uint32_t code = GetHardwareCode();
    auto it = axisValues.find(code);
    if (it == axisValues.end()) return 0.0f;

    float raw = it->second;

    // 1. Calibration (Map raw to 0.0 ... 1.0 based on user range)
    float norm = 0.0f;
    float range = m_rangeMax - m_rangeMin;
    if (std::abs(range) > 0.001f) {
        norm = (raw - m_rangeMin) / range;
    } else {
        norm = raw;
    }
    norm = (norm < 0.0f) ? 0.0f : (norm > 1.0f ? 1.0f : norm); // Early clamp

    // 2. Side Selection and Mapping to Action Magnitude
    bool isCentered = (m_rangeMin < -0.1f);
    float magnitude = 0.0f;
    bool isNegative = false;

    if (m_side == "positive") {
        if (isCentered) {
            magnitude = (norm > 0.5f) ? (norm - 0.5f) * 2.0f : 0.0f;
        } else {
            magnitude = norm;
        }
    } else if (m_side == "negative") {
        if (isCentered) {
            magnitude = (norm < 0.5f) ? (0.5f - norm) * 2.0f : 0.0f;
        } else {
            magnitude = 1.0f - norm;
        }
    } else { // "both"
        if (isCentered) {
            float centeredVal = norm * 2.0f - 1.0f;
            magnitude = (centeredVal < 0.0f) ? -centeredVal : centeredVal;
            isNegative = (centeredVal < 0.0f);
        } else {
            magnitude = norm;
        }
    }

    // 3. Apply Deadzone and Saturation
    float processed = 0.0f;
    float safeSaturation = (m_saturation > m_deadzone + 0.01f) ? m_saturation : m_deadzone + 0.01f;
    
    if (magnitude > m_deadzone) {
        processed = (magnitude - m_deadzone) / (safeSaturation - m_deadzone);
        processed = (processed > 1.0f) ? 1.0f : processed;
    }

    // 4. Apply Sensitivity and Curves
    processed *= m_sensitivity;
    if (m_curve == "exponential") processed = processed * processed;
    else if (m_curve == "logarithmic") processed = std::sqrt(processed);
    else if (m_curve == "s-curve") processed = processed * processed * (3.0f - 2.0f * processed);

    // 5. Digital Emulation
    if (m_mode == "digital") {
        float digitalVal = (processed >= m_threshold) ? 1.0f : 0.0f;
        if (m_invert) digitalVal = 1.0f - digitalVal;
        return digitalVal;
    }

    // 6. Final Result
    float finalVal = isNegative ? -processed : processed;
    
    // Systemic Trigger Handling: Triggers are always 0..1 and don't need 'side' splitting
    bool isTrigger = (m_axis == System::GamepadButton::LeftTriggerAxis || m_axis == System::GamepadButton::RightTriggerAxis);

    if (m_invert) {
        if (isTrigger) {
            finalVal = 1.0f - processed;
        } else {
            finalVal *= -1.0f;
        }
    }
    
    // Safety clamp
    float minLimit = (m_side == "both" && isCentered && !isTrigger) ? -1.0f : 0.0f;
    finalVal = (finalVal < minLimit) ? minLimit : (finalVal > 1.0f ? 1.0f : finalVal);

    if (m_smoothing > 0.001f) {
        float alpha = 1.0f - (m_smoothing > 0.99f ? 0.99f : m_smoothing);
        finalVal = m_lastValue + alpha * (finalVal - m_lastValue);
    }
    
    // --- DEBUG LOGGING ---
    // if (std::abs(finalVal - m_lastValue) > 0.001f) {
    //     auto logger = Logging::LoggerFactory::GetInstance().GetLogger("AxisDebug");
    //     logger->Debug("[Gamepad] Axis {}: Raw={:.4f} -> Final={:.4f} (Side={}, Mode={})", 
    //         GamepadButtonMapping::GetInstance().GetButtonName(m_axis), raw, finalVal, m_side, m_mode);
    // }

    m_lastValue = finalVal;
    return finalVal;
}

bool GamepadAxisInput::InvolvesHardwareCode(uint32_t code) const {
    return GetHardwareCode() == code;
}

bool GamepadAxisInput::IsSameAs(const IBindableInput& other) const {
    if (other.GetType() != InputType::GamepadAxis) return false;
    const auto& axisInput = static_cast<const GamepadAxisInput&>(other);
    return m_axis == axisInput.m_axis;
}

nlohmann::ordered_json GamepadAxisInput::ToJson() const {
    nlohmann::ordered_json j;
    j["type"] = "gamepad_axis";
    j["key"] = GamepadButtonMapping::GetInstance().GetButtonName(m_axis);
    j["mode"] = m_mode;
    
    if (m_mode == "analog") {
        j["curve"] = m_curve;
        j["side"] = m_side;
        j["invert"] = m_invert;
        j["deadzone"] = m_deadzone;
        j["saturation"] = m_saturation;
        j["sensitivity"] = m_sensitivity;
        j["smoothing"] = m_smoothing;
        j["range_min"] = m_rangeMin;
        j["range_max"] = m_rangeMax;
    } else {
        j["threshold"] = m_threshold;
    }

    return j;
}

std::string GamepadAxisInput::GetDisplayName() const {
    auto detectedType = Input::InputManager::GetInstance().GetDetectedGamepadType();
    return GamepadButtonMapping::GetInstance().GetButtonDisplayName(m_axis, detectedType);
}

bool GamepadAxisInput::IsValid() const {
    return m_axis != GamepadButton::Unknown;
}

}  // namespace Modules
SPF_NS_END
