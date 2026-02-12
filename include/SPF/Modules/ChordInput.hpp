#pragma once

#include "SPF/Modules/IBindableInput.hpp"
#include <vector>
#include <memory>

SPF_NS_BEGIN
namespace Modules {

/**
 * @class ChordInput
 * @brief Represents a combination of multiple physical inputs (a "chord").
 * 
 * This class acts as a container for multiple IBindableInput objects. 
 * An action is triggered only when all constituent inputs are active.
 */
class ChordInput : public IBindableInput {
 public:
  ChordInput();
  ~ChordInput() override;

  void AddInput(std::unique_ptr<IBindableInput> input);
  const std::vector<std::unique_ptr<IBindableInput>>& GetInputs() const { return m_inputs; }
  std::vector<uint32_t> GetConstituentHardwareCodes() const;

  // --- IBindableInput Overrides ---
  
  // Note: For chords, triggering logic is more complex and handled by KeyBindsManager 
  // checking the full set of pressed keys. These are kept for interface compatibility.
  bool IsTriggeredBy(const Input::KeyboardEvent& event) const override;
  bool IsTriggeredBy(const Input::GamepadEvent& event) const override;
  bool IsTriggeredBy(const Input::MouseButtonEvent& event) const override;
  bool IsTriggeredBy(const Input::JoystickEvent& event) const override;

  bool IsActive(const std::set<uint32_t>& pressedCodes) const override;
  bool InvolvesHardwareCode(uint32_t code) const override;

  bool IsSameAs(const IBindableInput& other) const override;
  float GetValue(const std::set<uint32_t>& pressedHardwareCodes, const std::map<uint32_t, float>& axisValues) const override;
  nlohmann::ordered_json ToJson() const override;
  std::string GetDisplayName() const override;
  bool IsValid() const override;
  InputType GetType() const override { return InputType::Chord; }
  uint32_t GetHardwareCode() const override { return 0; } // Chords don't have a single code

 private:
  std::vector<std::unique_ptr<IBindableInput>> m_inputs;
};

}  // namespace Modules
SPF_NS_END
