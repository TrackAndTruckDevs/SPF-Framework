#include "SPF/Modules/API/KeyBindsApi.hpp"
#include "SPF/Modules/PluginManager.hpp"
#include "SPF/Handles/KeyBindsHandle.hpp"
#include "SPF/Modules/KeyBindsManager.hpp"
#include "SPF/Modules/HandleManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

SPF_NS_BEGIN
namespace Modules::API {

SPF_KeyBinds_Handle* KeyBindsApi::Kbd_GetContext(const char* pluginName) {
    auto& pm = PluginManager::GetInstance();
    if (!pluginName || !pm.GetHandleManager()) return nullptr;
    auto handle = std::make_unique<Handles::KeyBindsHandle>(pluginName);
    return reinterpret_cast<SPF_KeyBinds_Handle*>(pm.GetHandleManager()->RegisterHandle(pluginName, std::move(handle)));
}

void KeyBindsApi::Kbd_Register(SPF_KeyBinds_Handle* handle, const char* actionName, void (*callback)(void)) {
    if (!handle || !actionName || !callback) return;
    auto& pm = PluginManager::GetInstance();
    if (pm.GetKeyBindsManager()) {
        pm.GetKeyBindsManager()->RegisterAction(actionName, callback);
    }
}

void KeyBindsApi::Kbd_UnregisterAll(SPF_KeyBinds_Handle* handle) {
    if (!handle) return;
    auto* kbdHandle = reinterpret_cast<Handles::KeyBindsHandle*>(handle);
    auto& pm = PluginManager::GetInstance();
    if (!pm.GetKeyBindsManager()) {
        auto logger = Logging::LoggerFactory::GetInstance().GetLogger("PluginManager");
        if (logger) logger->Error("Kbd_UnregisterAll: m_keyBindsManager is null. KeyBindsManager was not initialized before calling Kbd_UnregisterAll for plugin '{}'.", kbdHandle->pluginName);
        return;
    }
    pm.GetKeyBindsManager()->UnregisterOwner(kbdHandle->pluginName);
}

void KeyBindsApi::FillKeyBindsApi(SPF_KeyBinds_API* api) {
    if (!api) return;

    api->GetContext = &KeyBindsApi::Kbd_GetContext;
    api->Register = &KeyBindsApi::Kbd_Register;
    api->UnregisterAll = &KeyBindsApi::Kbd_UnregisterAll;
}

} // namespace Modules::API
SPF_NS_END
