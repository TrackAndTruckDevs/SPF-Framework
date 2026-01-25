#include "SPF/Modules/API/GameLogApi.hpp"
#include "SPF/Modules/GameLogEventManager.hpp"
#include "SPF/Handles/GameLogCallbackHandle.hpp" // Include the new handle
#include "SPF/Modules/PluginManager.hpp" // For PluginManager::GetInstance()
#include "SPF/Modules/HandleManager.hpp" // For HandleManager definition

SPF_NS_BEGIN
namespace Modules::API {

SPF_GameLog_Handle* GameLogApi::GLog_GetContext(const char* pluginName) {
    // For GameLog, the context is just the plugin name. We use it to group handles.
    // We return the pluginName cast to a handle as a simple identifier.
    return reinterpret_cast<SPF_GameLog_Handle*>(const_cast<char*>(pluginName));
}

SPF_GameLog_Callback_Handle* GameLogApi::GLog_RegisterCallback(SPF_GameLog_Handle* h, SPF_GameLog_Callback_t callback, void* userData) {
    if (!h || !callback) return nullptr;
    const char* pluginName = reinterpret_cast<const char*>(h);

    // Register the callback with the event manager
    GameLogEventManager::GetInstance().RegisterCallback(callback, userData);

    // Create a new handle on the heap. This handle will be owned by the framework.
    // Its destructor will automatically unregister the callback.
    auto unique_h = std::make_unique<Handles::GameLogCallbackHandle>(callback, userData);
    
    // Register the handle with the HandleManager for RAII management
    return reinterpret_cast<SPF_GameLog_Callback_Handle*>(
        PluginManager::GetInstance().GetHandleManager()->RegisterHandle(pluginName, std::move(unique_h))
    );
}

void GameLogApi::FillGameLogApi(SPF_GameLog_API* api) {
    if (!api) return;
    api->GLog_GetContext = &GameLogApi::GLog_GetContext;
    api->GLog_RegisterCallback = &GameLogApi::GLog_RegisterCallback;
}

} // namespace Modules::API
SPF_NS_END
