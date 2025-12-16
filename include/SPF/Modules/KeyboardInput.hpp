#pragma once

#include "SPF/Modules/IBindableInput.hpp"
#include "SPF/System/Keyboard.hpp"
#include "SPF/Input/InputEvents.hpp" // For Input::PressType
#include <nlohmann/json.hpp>

SPF_NS_BEGIN
namespace Modules {
class KeyboardInput : public IBindableInput {
 public:
  explicit KeyboardInput(const nlohmann::json& config);

  bool IsTriggeredBy(const Input::KeyboardEvent& event) const override;
  bool IsTriggeredBy(System::Keyboard key) const override;
  nlohmann::json ToJson() const override;
  std::string GetDisplayName() const override;
  bool IsValid() const override;
  InputType GetType() const override;

  bool IsSameAs(const IBindableInput& other) const override;

 private:
  System::Keyboard m_key;
  Input::PressType m_pressType;
};

}  // namespace Modules
SPF_NS_END