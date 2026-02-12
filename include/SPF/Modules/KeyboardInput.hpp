#pragma once

#include "SPF/Modules/IBindableInput.hpp"
#include "SPF/System/Keyboard.hpp"
#include "SPF/Input/InputEvents.hpp" // For Input::PressType
#include <nlohmann/json.hpp>

SPF_NS_BEGIN
namespace Modules {
class KeyboardInput : public IBindableInput {
 public:
  explicit KeyboardInput(const nlohmann::ordered_json& config);

  bool IsTriggeredBy(const Input::KeyboardEvent& event) const override;
  bool IsTriggeredBy(System::Keyboard key) const override;
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
  System::Keyboard m_key;
};

}  // namespace Modules
SPF_NS_END