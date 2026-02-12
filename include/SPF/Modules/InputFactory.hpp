#pragma once
#include "SPF/Modules/IBindableInput.hpp"
#include "SPF/Modules/KeyboardInput.hpp"
#include "SPF/Modules/GamepadInput.hpp"
#include "SPF/Modules/GamepadAxisInput.hpp"
#include "SPF/Modules/MouseInput.hpp"
#include "SPF/Modules/MouseAxisInput.hpp"
#include "SPF/Modules/JoystickInput.hpp"
#include "SPF/Modules/JoystickAxisInput.hpp"
#include "SPF/Modules/ChordInput.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include <nlohmann/json.hpp>
#include <memory>
#include <string>

SPF_NS_BEGIN
namespace Modules {
class InputFactory {
 public:
  static std::unique_ptr<IBindableInput> CreateFromJson(const nlohmann::ordered_json& configJson) {
    if (!configJson.is_object() || !configJson.contains("type")) {
      return nullptr;
    }

    std::string type = configJson.value("type", "");
    auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger("InputFactory");

    if (type == "keyboard") {
      auto input = std::make_unique<KeyboardInput>(configJson);
      if (input && input->IsValid()) {
        return input;
      } else {
        logger->Warn("Validation failed for keyboard input: {}", configJson.dump());
        return nullptr;
      }
    } else if (type == "gamepad") {
      auto input = std::make_unique<GamepadInput>(configJson);
      if (input && input->IsValid()) {
        return input;
      } else {
        logger->Warn("Validation failed for gamepad input: {}", configJson.dump());
        return nullptr;
      }
    } else if (type == "gamepad_axis") {
        auto input = std::make_unique<GamepadAxisInput>(configJson);
        if (input && input->IsValid()) {
            return input;
        } else {
            logger->Warn("Validation failed for gamepad axis input: {}", configJson.dump());
            return nullptr;
        }
    } else if (type == "mouse") {
        auto input = std::make_unique<MouseInput>(configJson);
        if (input && input->IsValid()) {
            return input;
        } else {
            logger->Warn("Validation failed for mouse input: {}. Note: Left mouse button cannot be bound.", configJson.dump());
            return nullptr;
        }
    } else if (type == "mouse_axis") {
        auto input = std::make_unique<MouseAxisInput>(configJson);
        if (input && input->IsValid()) {
            return input;
        } else {
            logger->Warn("Validation failed for mouse axis input: {}", configJson.dump());
            return nullptr;
        }
    } else if (type == "joystick") {
        auto input = std::make_unique<JoystickInput>(configJson);
        if (input && input->IsValid()) {
            return input;
        } else {
            logger->Warn("Validation failed for joystick input: {}", configJson.dump());
            return nullptr;
        }
    } else if (type == "joystick_axis") {
        auto input = std::make_unique<JoystickAxisInput>(configJson);
        if (input && input->IsValid()) {
            return input;
        } else {
            logger->Warn("Validation failed for joystick axis input: {}", configJson.dump());
            return nullptr;
        }
    } else if (type == "chord") {
        if (!configJson.contains("bindings") || !configJson["bindings"].is_array()) {
            logger->Warn("Chord input missing 'bindings' array: {}", configJson.dump());
            return nullptr;
        }
        auto chord = std::make_unique<ChordInput>();
        for (const auto& innerJson : configJson["bindings"]) {
            if (!innerJson.is_object()) continue;
            auto innerInput = CreateFromJson(innerJson);
            if (innerInput) {
                chord->AddInput(std::move(innerInput));
            }
        }
        if (chord->IsValid()) {
            return chord;
        } else {
            logger->Warn("Validation failed for chord input: {}", configJson.dump());
            return nullptr;
        }
    }

    // This part is now only reached if the type is genuinely unknown
    logger->Warn("Unknown bindable input type: '{}'", type);
    return nullptr;
  }
};
}  // namespace Modules
SPF_NS_END