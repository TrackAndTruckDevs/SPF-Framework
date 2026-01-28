#include "SPF/Modules/API/KeyBindsApi.hpp"
#include "SPF/Modules/PluginManager.hpp"
#include "SPF/Handles/KeyBindsHandle.hpp"
#include "SPF/Modules/KeyBindsManager.hpp"
#include "SPF/Modules/HandleManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

SPF_NS_BEGIN
namespace Modules::API {

SPF_KeyBinds_Handle* KeyBindsApi::Kbind_GetContext(const char* pluginName) {
    auto& pm = PluginManager::GetInstance();
    if (!pluginName || !pm.GetHandleManager()) return nullptr;
    auto unique_h = std::make_unique<Handles::KeyBindsHandle>(pluginName);
    return reinterpret_cast<SPF_KeyBinds_Handle*>(pm.GetHandleManager()->RegisterHandle(pluginName, std::move(unique_h)));
}

void KeyBindsApi::Kbind_Register(SPF_KeyBinds_Handle* h, const char* actionName, void (*callback)(void)) {
    if (!h || !actionName || !callback) return;
    auto& pm = PluginManager::GetInstance();
    if (pm.GetKeyBindsManager()) {
        pm.GetKeyBindsManager()->RegisterAction(actionName, callback);
    }
}

void KeyBindsApi::Kbind_UnregisterAll(SPF_KeyBinds_Handle* h) {
    if (!h) return;
    auto* kbdHandle = reinterpret_cast<Handles::KeyBindsHandle*>(h);
    auto& pm = PluginManager::GetInstance();
    if (!pm.GetKeyBindsManager()) {
        auto logger = Logging::LoggerFactory::GetInstance().GetLogger("PluginManager");
        if (logger) logger->Error("Kbind_UnregisterAll: m_keyBindsManager is null. KeyBindsManager was not initialized before calling Kbind_UnregisterAll for plugin '{}'.", kbdHandle->pluginName);
        return;
    }
    pm.GetKeyBindsManager()->UnregisterOwner(kbdHandle->pluginName);
}

void KeyBindsApi::Kbind_SetBlockState(SPF_KeyBinds_Handle* h, const char* actionName, bool block) {
    if (!h || !actionName) return;
    auto& pm = PluginManager::GetInstance();
    if (pm.GetKeyBindsManager()) {
        pm.GetKeyBindsManager()->SetBlockState(actionName, block);
    }
}

void KeyBindsApi::FillKeyBindsApi(SPF_KeyBinds_API* api) {
    if (!api) return;

    api->Kbind_GetContext = &KeyBindsApi::Kbind_GetContext;
    api->Kbind_Register = &KeyBindsApi::Kbind_Register;
    api->Kbind_UnregisterAll = &KeyBindsApi::Kbind_UnregisterAll;
    api->Kbind_SetBlockState = &KeyBindsApi::Kbind_SetBlockState;
}

} // namespace Modules::API
SPF_NS_END
