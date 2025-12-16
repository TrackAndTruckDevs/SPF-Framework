#pragma once

#include "SPF/Modules/IBindableInput.hpp"
#include "SPF/System/GamepadButton.hpp"
#include "SPF/Input/InputEvents.hpp" // For Input::PressType
#include <nlohmann/json.hpp>

SPF_NS_BEGIN
namespace Modules {
class GamepadInput : public IBindableInput {
 public:
  explicit GamepadInput(const nlohmann::json& config);

  // IBindableInput implementation
  bool IsTriggeredBy(const Input::GamepadEvent& event) const override;
  nlohmann::json ToJson() const override;
  std::string GetDisplayName() const override;
  bool IsValid() const override;
  InputType GetType() const override;

  bool IsSameAs(const IBindableInput& other) const override;

 private:
  System::GamepadButton m_button;
  Input::PressType m_pressType;
};

}  // namespace Modules
SPF_NS_END
