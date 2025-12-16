#pragma once

#include "SPF/Input/InputEvents.hpp"
#include <map>
#include <string>

SPF_NS_BEGIN

namespace Config {
// Enum definitions that are used across UI and backend for configuration.
// Moved here for centralization.

enum class ConsumptionPolicy {
  Never,      // Always pass the input to the game (default behavior)
  OnUIFocus,  // Consume the input only when an interactive UI window is focused
  Always      // Always consume the input and never pass it to the game
};

// --- Centralized Definitions for Binding Properties ---

struct PressTypeInfo {
  std::string string_id;
  std::string loc_key;
};

static inline const std::map<Input::PressType, PressTypeInfo> PressTypeMap = {{Input::PressType::Short, {"short", "enums.press_type.short"}},
                                                                              {Input::PressType::Long, {"long", "enums.press_type.long"}}};

struct ConsumptionPolicyInfo {
  std::string string_id;
  std::string loc_key;
};

static inline const std::map<ConsumptionPolicy, ConsumptionPolicyInfo> ConsumptionPolicyMap = {
    {ConsumptionPolicy::Never, {"never", "enums.consumption_policy.never"}},
    {ConsumptionPolicy::OnUIFocus, {"on_ui_focus", "enums.consumption_policy.on_ui_focus"}},
    {ConsumptionPolicy::Always, {"always", "enums.consumption_policy.always"}}};
}  // namespace Config

SPF_NS_END
