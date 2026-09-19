#include "SPF/Modules/InputDisplay.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Config/IConfigService.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Modules/ChordInput.hpp"
#include "SPF/SPF_API/SPF_Icons.h"

#include "fmt/format.h"

#include <cstddef>

SPF_NS_BEGIN
namespace Modules {

const char* GetIconForInputType(InputType type) {
  switch (type) {
    case InputType::Keyboard:
      return ICON_FA_KEYBOARD;
    case InputType::Gamepad:
    case InputType::GamepadAxis:
    case InputType::Joystick:
    case InputType::JoystickAxis:
      return ICON_FA_GAMEPAD;
    case InputType::Mouse:
    case InputType::MouseAxis:
      return ICON_FA_COMPUTER_MOUSE;
    default:
      return "";
  }
}

std::string GetDisplayNameWithIcon(const IBindableInput& input) {
  if (input.GetType() == InputType::Chord) {
    if (const auto* chord = dynamic_cast<const ChordInput*>(&input)) {
      std::string text;
      const auto& constituents = chord->GetInputs();
      InputType lastType = InputType::Chord;  // Sentinel: no constituent is ever a chord itself.
      for (size_t i = 0; i < constituents.size(); ++i) {
        InputType currentType = constituents[i]->GetType();
        if (currentType != lastType) {
          // Add icon only when the device type changes along the chord.
          text += fmt::format("{} ", GetIconForInputType(currentType));
          lastType = currentType;
        }
        text += constituents[i]->GetDisplayName();
        if (i + 1 < constituents.size()) text += " + ";
      }
      return text;
    }
  }

  return fmt::format("{} {}", GetIconForInputType(input.GetType()), input.GetDisplayName());
}

std::string GetTranslatedActionName(Config::IConfigService& configService, const std::string& fullActionName) {
  size_t lastDot = fullActionName.rfind('.');
  if (lastDot == std::string::npos) {
    return fullActionName;  // Cannot parse, return raw name
  }
  std::string group = fullActionName.substr(0, lastDot);
  std::string actionName = fullActionName.substr(lastDot + 1);

  auto& loc = Localization::LocalizationManager::GetInstance();
  const auto* keybindsConfig = configService.GetMergedConfig("keybinds");
  if (keybindsConfig && keybindsConfig->contains(group)) {
    const auto& groupObject = (*keybindsConfig)[group];
    if (groupObject.contains(actionName)) {
      const auto& actionObject = groupObject[actionName];
      if (actionObject.is_object() && actionObject.contains("_meta")) {
        const auto& meta = actionObject["_meta"];
        if (meta.contains("titleKey") && meta["titleKey"].is_string()) {
          const auto& titleKey = meta["titleKey"].get<std::string>();
          if (!titleKey.empty()) {
            size_t firstDot = group.find('.');
            std::string owner = (firstDot != std::string::npos) ? group.substr(0, firstDot) : group;
            std::string translated = loc.Get(owner, titleKey);
            if (translated != titleKey) {
              return translated;
            }
          }
        }
      }
    }
  }

  return fullActionName;
}

}  // namespace Modules
SPF_NS_END
