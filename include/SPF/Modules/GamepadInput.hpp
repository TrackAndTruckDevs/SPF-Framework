#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Input/InputEvents.hpp"  // For Input::PressType
#include "SPF/Modules/IBindableInput.hpp"
#include "SPF/System/GamepadButton.hpp"

#include "nlohmann/json.hpp"  // IWYU pragma: keep
#include "nlohmann/json_fwd.hpp"

#include <cstdint>
#include <map>
#include <set>
#include <string>

SPF_NS_BEGIN
namespace Modules {
class GamepadInput : public IBindableInput {
 public:
  explicit GamepadInput(const nlohmann::ordered_json& config);

  // IBindableInput implementation
  bool IsTriggeredBy(const Input::GamepadEvent& event) const override;
  nlohmann::ordered_json ToJson() const override;
  std::string GetDisplayName() const override;
  bool IsValid() const override;
  InputType GetType() const override;
  uint32_t GetHardwareCode() const override;
  bool IsActive(const std::set<uint32_t>& pressedCodes) const override;
  bool InvolvesHardwareCode(uint32_t code) const override;

  bool IsSameAs(const IBindableInput& other) const override;
  float GetValue(const std::set<uint32_t>& pressedHardwareCodes, const std::map<uint32_t, float>& axisValues) const override;

 private:
  System::GamepadButton m_button;
};

}  // namespace Modules
SPF_NS_END
