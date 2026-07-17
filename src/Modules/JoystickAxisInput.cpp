#include "SPF/Modules/JoystickAxisInput.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Modules/IBindableInput.hpp"

#include "fmt/format.h"
#include "nlohmann/json_fwd.hpp"

#include <cmath>
#include <cstdint>
#include <fmt/core.h>
#include <map>
#include <set>
#include <string>

SPF_NS_BEGIN
namespace Modules {

JoystickAxisInput::JoystickAxisInput(const nlohmann::ordered_json& config) {
  if (config.contains("key")) {
    if (config["key"].is_string()) {
      try {
        m_axisIndex = std::stoi(config["key"].get<std::string>());
      } catch (...) {
        m_axisIndex = 0;
      }
    } else {
      m_axisIndex = config.value("key", 0);
    }
  } else {
    m_axisIndex = 0;
  }

  m_mode = config.value("mode", "analog");
  m_deadzone = config.value("deadzone", 0.0f);
  m_saturation = config.value("saturation", 1.0f);
  m_sensitivity = config.value("sensitivity", 1.0f);
  m_threshold = config.value("threshold", 0.5f);
  m_invert = config.value("invert", false);
  m_curve = config.value("curve", "linear");
  m_side = config.value("side", "both");
  m_smoothing = config.value("smoothing", 0.0f);
  m_rangeMin = config.value("range_min", -1.0f);
  m_rangeMax = config.value("range_max", 1.0f);
}

uint32_t JoystickAxisInput::GetHardwareCode() const { return 0x04010000 | static_cast<uint32_t>(m_axisIndex); }

bool JoystickAxisInput::IsActive(const std::set<uint32_t>& pressedCodes) const { return pressedCodes.count(GetHardwareCode()) > 0; }

float JoystickAxisInput::GetValue(const std::set<uint32_t>& pressedHardwareCodes, const std::map<uint32_t, float>& axisValues) const {
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
  norm = (norm < 0.0f) ? 0.0f : (norm > 1.0f ? 1.0f : norm);

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
  } else {  // "both"
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
  if (m_curve == "exponential")
    processed = processed * processed;
  else if (m_curve == "logarithmic")
    processed = std::sqrt(processed);
  else if (m_curve == "s-curve")
    processed = processed * processed * (3.0f - 2.0f * processed);

  // 5. Digital Emulation
  if (m_mode == "digital") {
    float digitalVal = (processed >= m_threshold) ? 1.0f : 0.0f;
    if (m_invert) digitalVal = 1.0f - digitalVal;
    return digitalVal;
  }

  // 6. Final Result
  float finalVal = isNegative ? -processed : processed;
  if (m_invert) finalVal *= -1.0f;

  float minLimit = (m_side == "both" && isCentered) ? -1.0f : 0.0f;
  finalVal = (finalVal < minLimit) ? minLimit : (finalVal > 1.0f ? 1.0f : finalVal);

  if (m_smoothing > 0.001f) {
    float alpha = 1.0f - (m_smoothing > 0.99f ? 0.99f : m_smoothing);
    finalVal = m_lastValue + alpha * (finalVal - m_lastValue);
  }

  // --- DEBUG LOGGING ---
  // if (std::abs(finalVal - m_lastValue) > 0.001f) {
  //     auto logger = Logging::LoggerFactory::GetInstance().GetLogger("AxisDebug");
  //     logger->Debug("[Joystick] Axis {}: Raw={:.4f} -> Final={:.4f} (Side={}, Mode={})",
  //         m_axisIndex, raw, finalVal, m_side, m_mode);
  // }

  m_lastValue = finalVal;
  return finalVal;
}

bool JoystickAxisInput::InvolvesHardwareCode(uint32_t code) const { return GetHardwareCode() == code; }

bool JoystickAxisInput::IsSameAs(const IBindableInput& other) const {
  if (other.GetType() != InputType::JoystickAxis) return false;
  const auto& otherInput = static_cast<const JoystickAxisInput&>(other);
  return m_axisIndex == otherInput.m_axisIndex;
}

nlohmann::ordered_json JoystickAxisInput::ToJson() const {
  nlohmann::ordered_json j;
  j["type"] = "joystick_axis";
  j["key"] = std::to_string(m_axisIndex);
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

std::string JoystickAxisInput::GetDisplayName() const { return fmt::format("Joystick Axis {}", m_axisIndex); }

bool JoystickAxisInput::IsValid() const { return m_axisIndex >= 0 && m_axisIndex < 16; }

}  // namespace Modules
SPF_NS_END
