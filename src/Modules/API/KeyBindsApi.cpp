#include "SPF/Modules/API/KeyBindsApi.hpp"
#include "SPF/Modules/PluginManager.hpp"
#include "SPF/Handles/KeyBindsHandle.hpp"
#include "SPF/Modules/KeyBindsManager.hpp"
#include "SPF/Modules/HandleManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

SPF_NS_BEGIN
namespace Modules::API {

namespace {
/**
 * @brief Ensures action name starts with PluginID. (e.g. "test" -> "ExamplePlugin.test")
 */
std::string SanitizeName(const std::string& pluginName, const char* actionName) {
    if (!actionName) return "";
    std::string name = actionName;
    std::string prefix = pluginName + ".";
    if (name.find(prefix) != 0) {
        return prefix + name;
    }
    return name;
}
}

SPF_KeyBinds_Handle* KeyBindsApi::Kbind_GetContext(const char* pluginName) {
    auto& pm = PluginManager::GetInstance();
    if (!pluginName || !pm.GetHandleManager()) return nullptr;
    auto unique_h = std::make_unique<Handles::KeyBindsHandle>(pluginName);
    return reinterpret_cast<SPF_KeyBinds_Handle*>(pm.GetHandleManager()->RegisterHandle(pluginName, std::move(unique_h)));
}

void KeyBindsApi::Kbind_Register(SPF_KeyBinds_Handle* h, const char* actionName, void (*callback)(void)) {
    if (!h || !actionName || !callback) return;
    auto* kbdHandle = reinterpret_cast<Handles::KeyBindsHandle*>(h);
    std::string fullActionName = SanitizeName(kbdHandle->pluginName, actionName);
    auto& pm = PluginManager::GetInstance();
    if (pm.GetKeyBindsManager()) {
        pm.GetKeyBindsManager()->RegisterAction(fullActionName, callback);
    }
}

void KeyBindsApi::Kbind_Register_Ex(SPF_KeyBinds_Handle* h, const char* actionName, SPF_Keybind_Callback_Ex callback, void* user_data) {
    if (!h || !actionName || !callback) return;
    auto* kbdHandle = reinterpret_cast<Handles::KeyBindsHandle*>(h);
    std::string fullActionName = SanitizeName(kbdHandle->pluginName, actionName);
    auto& pm = PluginManager::GetInstance();
    if (pm.GetKeyBindsManager()) {
        pm.GetKeyBindsManager()->RegisterActionEx(fullActionName, [callback](const std::string& id, void* data) {
            callback(id.c_str(), data);
        }, user_data);
    }
}

void KeyBindsApi::Kbind_UnregisterAll(SPF_KeyBinds_Handle* h) {
    if (!h) return;
    auto* kbdHandle = reinterpret_cast<Handles::KeyBindsHandle*>(h);
    auto& pm = PluginManager::GetInstance();
    if (!pm.GetKeyBindsManager()) return;
    pm.GetKeyBindsManager()->UnregisterOwner(kbdHandle->pluginName);
}

void KeyBindsApi::Kbind_SetBlockState(SPF_KeyBinds_Handle* h, const char* actionName, bool block) {
    if (!h || !actionName) return;
    auto* kbdHandle = reinterpret_cast<Handles::KeyBindsHandle*>(h);
    std::string fullActionName = SanitizeName(kbdHandle->pluginName, actionName);
    auto& pm = PluginManager::GetInstance();
    if (pm.GetKeyBindsManager()) {
        pm.GetKeyBindsManager()->SetBlockState(fullActionName, block);
    }
}

float KeyBindsApi::Kbind_GetActionValue(SPF_KeyBinds_Handle* h, const char* actionName) {
    if (!h || !actionName) return 0.0f;
    auto* kbdHandle = reinterpret_cast<Handles::KeyBindsHandle*>(h);
    std::string fullActionName = SanitizeName(kbdHandle->pluginName, actionName);
    auto& pm = PluginManager::GetInstance();
    if (pm.GetKeyBindsManager()) {
        return pm.GetKeyBindsManager()->GetActionValue(fullActionName);
    }
    return 0.0f;
}

int KeyBindsApi::Kbind_GetBindingCount(SPF_KeyBinds_Handle* h, const char* actionName) {
    if (!h || !actionName) return 0;
    auto* kbdHandle = reinterpret_cast<Handles::KeyBindsHandle*>(h);
    std::string fullActionName = SanitizeName(kbdHandle->pluginName, actionName);
    auto& pm = PluginManager::GetInstance();
    if (pm.GetKeyBindsManager()) {
        return static_cast<int>(pm.GetKeyBindsManager()->GetBindingCount(fullActionName));
    }
    return 0;
}

SPF_BindingType KeyBindsApi::Kbind_GetBindingType(SPF_KeyBinds_Handle* h, const char* actionName, int index) {
    if (!h || !actionName) return SPF_BINDING_UNKNOWN;
    auto* kbdHandle = reinterpret_cast<Handles::KeyBindsHandle*>(h);
    std::string fullActionName = SanitizeName(kbdHandle->pluginName, actionName);
    auto& pm = PluginManager::GetInstance();
    auto* kbm = pm.GetKeyBindsManager();
    if (!kbm) return SPF_BINDING_UNKNOWN;

    auto* binding = kbm->GetBinding(fullActionName, static_cast<size_t>(index));
    if (!binding || !binding->Input) return SPF_BINDING_UNKNOWN;

    switch (binding->Input->GetType()) {
        case Modules::InputType::Keyboard:     return SPF_BINDING_KEYBOARD;
        case Modules::InputType::Gamepad:      return SPF_BINDING_GAMEPAD;
        case Modules::InputType::Mouse:        return SPF_BINDING_MOUSE;
        case Modules::InputType::Joystick:     return SPF_BINDING_JOYSTICK;
        case Modules::InputType::Chord:        return SPF_BINDING_CHORD;
        case Modules::InputType::GamepadAxis:  return SPF_BINDING_GAMEPAD_AXIS;
        case Modules::InputType::MouseAxis:    return SPF_BINDING_MOUSE_AXIS;
        case Modules::InputType::JoystickAxis: return SPF_BINDING_JOYSTICK_AXIS;
        default:                               return SPF_BINDING_UNKNOWN;
    }
}

SPF_ActivationBehavior KeyBindsApi::Kbind_GetBindingBehavior(SPF_KeyBinds_Handle* h, const char* actionName, int index) {
    if (!h || !actionName) return SPF_BEHAVIOR_NA;
    auto* kbdHandle = reinterpret_cast<Handles::KeyBindsHandle*>(h);
    std::string fullActionName = SanitizeName(kbdHandle->pluginName, actionName);
    auto& pm = PluginManager::GetInstance();
    auto* kbm = pm.GetKeyBindsManager();
    if (!kbm) return SPF_BEHAVIOR_NA;

    auto* binding = kbm->GetBinding(fullActionName, static_cast<size_t>(index));
    if (!binding || !binding->Input) return SPF_BEHAVIOR_NA;

    auto type = binding->Input->GetType();
    bool isAxis = (type == Modules::InputType::GamepadAxis || type == Modules::InputType::MouseAxis || type == Modules::InputType::JoystickAxis);
    if (isAxis) {
        std::string mode = binding->originalBindingJson.value("mode", "analog");
        if (mode != "digital") return SPF_BEHAVIOR_NA;
    }

    return (binding->Behavior == Modules::ActivationBehavior::Toggle) ? SPF_BEHAVIOR_TOGGLE : SPF_BEHAVIOR_HOLD;
}

SPF_PressType KeyBindsApi::Kbind_GetBindingPressType(SPF_KeyBinds_Handle* h, const char* actionName, int index) {
    if (!h || !actionName) return SPF_PRESS_NA;
    auto* kbdHandle = reinterpret_cast<Handles::KeyBindsHandle*>(h);
    std::string fullActionName = SanitizeName(kbdHandle->pluginName, actionName);
    auto& pm = PluginManager::GetInstance();
    auto* kbm = pm.GetKeyBindsManager();
    if (!kbm) return SPF_PRESS_NA;

    auto* binding = kbm->GetBinding(fullActionName, static_cast<size_t>(index));
    if (!binding || !binding->Input) return SPF_PRESS_NA;

    auto type = binding->Input->GetType();
    bool isAxis = (type == Modules::InputType::GamepadAxis || type == Modules::InputType::MouseAxis || type == Modules::InputType::JoystickAxis);
    if (isAxis) {
        std::string mode = binding->originalBindingJson.value("mode", "analog");
        if (mode != "digital") return SPF_PRESS_NA;
    }

    return (binding->PressType == Input::PressType::Long) ? SPF_PRESS_LONG : SPF_PRESS_SHORT;
}

SPF_InputMode KeyBindsApi::Kbind_GetBindingMode(SPF_KeyBinds_Handle* h, const char* actionName, int index) {
    if (!h || !actionName) return SPF_MODE_NA;
    auto* kbdHandle = reinterpret_cast<Handles::KeyBindsHandle*>(h);
    std::string fullActionName = SanitizeName(kbdHandle->pluginName, actionName);
    auto& pm = PluginManager::GetInstance();
    auto* kbm = pm.GetKeyBindsManager();
    if (!kbm) return SPF_MODE_NA;

    auto* binding = kbm->GetBinding(fullActionName, static_cast<size_t>(index));
    if (!binding || !binding->Input) return SPF_MODE_NA;

    auto type = binding->Input->GetType();
    bool isAxis = (type == Modules::InputType::GamepadAxis || type == Modules::InputType::MouseAxis || type == Modules::InputType::JoystickAxis);
    if (!isAxis) return SPF_MODE_NA;

    std::string mode = binding->originalBindingJson.value("mode", "analog");
    return (mode == "digital") ? SPF_MODE_DIGITAL : SPF_MODE_ANALOG;
}

SPF_AxisSide KeyBindsApi::Kbind_GetBindingSide(SPF_KeyBinds_Handle* h, const char* actionName, int index) {
    if (!h || !actionName) return SPF_SIDE_NA;
    auto* kbdHandle = reinterpret_cast<Handles::KeyBindsHandle*>(h);
    std::string fullActionName = SanitizeName(kbdHandle->pluginName, actionName);
    auto& pm = PluginManager::GetInstance();
    auto* kbm = pm.GetKeyBindsManager();
    if (!kbm) return SPF_SIDE_NA;

    auto* binding = kbm->GetBinding(fullActionName, static_cast<size_t>(index));
    if (!binding || !binding->Input) return SPF_SIDE_NA;

    auto type = binding->Input->GetType();
    bool isAxis = (type == Modules::InputType::GamepadAxis || type == Modules::InputType::MouseAxis || type == Modules::InputType::JoystickAxis);
    if (!isAxis) return SPF_SIDE_NA;

    std::string side = binding->originalBindingJson.value("side", "both");
    if (side == "positive") return SPF_SIDE_POSITIVE;
    if (side == "negative") return SPF_SIDE_NEGATIVE;
    return SPF_SIDE_BOTH;
}

SPF_AccumulatorMode KeyBindsApi::Kbind_GetBindingAccumulatorMode(SPF_KeyBinds_Handle* h, const char* actionName, int index) {
    if (!h || !actionName) return SPF_ACCUMULATOR_NA;
    auto* kbdHandle = reinterpret_cast<Handles::KeyBindsHandle*>(h);
    std::string fullActionName = SanitizeName(kbdHandle->pluginName, actionName);
    auto& pm = PluginManager::GetInstance();
    auto* kbm = pm.GetKeyBindsManager();
    if (!kbm) return SPF_ACCUMULATOR_NA;

    auto* binding = kbm->GetBinding(fullActionName, static_cast<size_t>(index));
    if (!binding || !binding->Input) return SPF_ACCUMULATOR_NA;

    auto type = binding->Input->GetType();
    bool isAxis = (type == Modules::InputType::GamepadAxis || type == Modules::InputType::MouseAxis || type == Modules::InputType::JoystickAxis);
    if (!isAxis) return SPF_ACCUMULATOR_NA;

    bool isAcc = (type == Modules::InputType::MouseAxis) || binding->originalBindingJson.value("accumulator", false);
    return isAcc ? SPF_ACCUMULATOR_ON : SPF_ACCUMULATOR_OFF;
}

int KeyBindsApi::Kbind_GetBindingName(SPF_KeyBinds_Handle* h, const char* actionName, int index, char* out_buffer, int buffer_size) {
    if (!h || !actionName || !out_buffer || buffer_size <= 0) return 0;
    auto* kbdHandle = reinterpret_cast<Handles::KeyBindsHandle*>(h);
    std::string fullActionName = SanitizeName(kbdHandle->pluginName, actionName);
    auto& pm = PluginManager::GetInstance();
    auto* kbm = pm.GetKeyBindsManager();
    if (!kbm) return 0;

    auto* binding = kbm->GetBinding(fullActionName, static_cast<size_t>(index));
    if (!binding || !binding->Input) return 0;

    std::string name = binding->Input->GetDisplayName();
    int len = static_cast<int>(name.copy(out_buffer, buffer_size - 1));
    out_buffer[len] = '\0';
    return len;
}

void KeyBindsApi::Kbind_RegisterActionMetadata(SPF_KeyBinds_Handle* h, const char* actionName, const char* titleKey, const char* descKey, SPF_Keybind_Callback_Ex callback, void* user_data) {
    if (!h || !actionName) return;
    auto* kbdHandle = reinterpret_cast<Handles::KeyBindsHandle*>(h);
    auto& pm = PluginManager::GetInstance();
    if (pm.GetConfigService()) {
        pm.GetConfigService()->RegisterActionMetadata(kbdHandle->pluginName, actionName, titleKey ? titleKey : "", descKey ? descKey : "");
    }
    if (callback && pm.GetKeyBindsManager()) {
        std::string fullActionName = SanitizeName(kbdHandle->pluginName, actionName);
        pm.GetKeyBindsManager()->RegisterActionEx(fullActionName, [callback](const std::string& id, void* data) {
            callback(id.c_str(), data);
        }, user_data);
    }
}

void KeyBindsApi::Kbind_UnregisterActionMetadata(SPF_KeyBinds_Handle* h, const char* actionName) {
    if (!h || !actionName) return;
    auto* kbdHandle = reinterpret_cast<Handles::KeyBindsHandle*>(h);
    auto& pm = PluginManager::GetInstance();
    if (pm.GetConfigService()) {
        pm.GetConfigService()->UnregisterActionMetadata(kbdHandle->pluginName, actionName);
    }
}

int KeyBindsApi::Kbind_GetActionCount(SPF_KeyBinds_Handle* h) {
    if (!h) return 0;
    auto* kbdHandle = reinterpret_cast<Handles::KeyBindsHandle*>(h);
    auto& pm = PluginManager::GetInstance();
    if (pm.GetConfigService()) {
        return static_cast<int>(pm.GetConfigService()->GetOwnedActions(kbdHandle->pluginName).size());
    }
    return 0;
}

int KeyBindsApi::Kbind_GetActionNameByIndex(SPF_KeyBinds_Handle* h, int index, char* out_buffer, int buffer_size) {
    if (!h || !out_buffer || buffer_size <= 0) return 0;
    auto* kbdHandle = reinterpret_cast<Handles::KeyBindsHandle*>(h);
    auto& pm = PluginManager::GetInstance();
    if (pm.GetConfigService()) {
        auto actions = pm.GetConfigService()->GetOwnedActions(kbdHandle->pluginName);
        if (index >= 0 && index < static_cast<int>(actions.size())) {
            const std::string& name = actions[index];
            int len = static_cast<int>(name.copy(out_buffer, buffer_size - 1));
            out_buffer[len] = '\0';
            return len;
        }
    }
    return 0;
}

void KeyBindsApi::FillKeyBindsApi(SPF_KeyBinds_API* api) {
    if (!api) return;

    api->Kbind_GetContext = &KeyBindsApi::Kbind_GetContext;
    api->Kbind_Register = &KeyBindsApi::Kbind_Register;
    api->Kbind_UnregisterAll = &KeyBindsApi::Kbind_UnregisterAll;
    api->Kbind_SetBlockState = &KeyBindsApi::Kbind_SetBlockState;
    api->Kbind_GetActionValue = &KeyBindsApi::Kbind_GetActionValue;

    api->Kbind_GetBindingCount = &KeyBindsApi::Kbind_GetBindingCount;
    api->Kbind_GetBindingType = &KeyBindsApi::Kbind_GetBindingType;
    api->Kbind_GetBindingBehavior = &KeyBindsApi::Kbind_GetBindingBehavior;
    api->Kbind_GetBindingPressType = &KeyBindsApi::Kbind_GetBindingPressType;
    api->Kbind_GetBindingMode = &KeyBindsApi::Kbind_GetBindingMode;
    api->Kbind_GetBindingSide = &KeyBindsApi::Kbind_GetBindingSide;
    api->Kbind_GetBindingAccumulatorMode = &KeyBindsApi::Kbind_GetBindingAccumulatorMode;
    api->Kbind_GetBindingName = &KeyBindsApi::Kbind_GetBindingName;

    api->Kbind_RegisterActionMetadata = &KeyBindsApi::Kbind_RegisterActionMetadata;
    api->Kbind_UnregisterActionMetadata = &KeyBindsApi::Kbind_UnregisterActionMetadata;
    api->Kbind_GetActionCount = &KeyBindsApi::Kbind_GetActionCount;
    api->Kbind_GetActionNameByIndex = &KeyBindsApi::Kbind_GetActionNameByIndex;
    api->Kbind_Register_Ex = &KeyBindsApi::Kbind_Register_Ex;
}

} // namespace Modules::API
SPF_NS_END
