#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Modules/IBindableInput.hpp"

#include "nlohmann/json_fwd.hpp"

#include <cstdint>
#include <map>
#include <set>
#include <string>

SPF_NS_BEGIN
namespace Modules {

class JoystickAxisInput : public IBindableInput {
 public:
  explicit JoystickAxisInput(const nlohmann::ordered_json& config);

  uint32_t GetHardwareCode() const override;
  bool IsActive(const std::set<uint32_t>& pressedCodes) const override;
  float GetValue(const std::set<uint32_t>& pressedHardwareCodes, const std::map<uint32_t, float>& axisValues) const override;
  bool InvolvesHardwareCode(uint32_t code) const override;
  bool IsSameAs(const IBindableInput& other) const override;
  nlohmann::ordered_json ToJson() const override;
  std::string GetDisplayName() const override;
  bool IsValid() const override;
  InputType GetType() const override { return InputType::JoystickAxis; }

 private:
  int m_axisIndex;
  std::string m_mode;  // "analog" or "digital"
  float m_deadzone = 0.0f;
  float m_saturation = 1.0f;
  float m_sensitivity = 1.0f;
  float m_threshold = 0.5f;
  std::string m_curve = "linear";
  std::string m_side = "both";
  float m_smoothing = 0.0f;
  float m_rangeMin = -1.0f;
  float m_rangeMax = 1.0f;
  bool m_invert = false;

  mutable float m_lastValue = 0.0f;  // For smoothing
};

}  // namespace Modules
SPF_NS_END
