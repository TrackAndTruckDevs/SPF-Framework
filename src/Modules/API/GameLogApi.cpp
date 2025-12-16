#include "SPF/Modules/API/GameLogApi.hpp"
#include "SPF/Modules/GameLogEventManager.hpp"
#include "SPF/Handles/GameLogCallbackHandle.hpp" // Include the new handle
#include "SPF/Modules/PluginManager.hpp" // For PluginManager::GetInstance()
#include "SPF/Modules/HandleManager.hpp" // For HandleManager definition

SPF_NS_BEGIN
namespace Modules::API {

SPF_GameLog_Callback_Handle GameLogApi::G_RegisterCallback(const char* pluginName, SPF_GameLog_Callback callback, void* user_data) {
    // Register the callback with the event manager
    GameLogEventManager::GetInstance().RegisterCallback(callback, user_data);

    // Create a new handle on the heap. This handle will be owned by the framework.
    // Its destructor will automatically unregister the callback.
    auto handle = std::make_unique<Handles::GameLogCallbackHandle>(callback, user_data);
    SPF_GameLog_Callback_Handle rawHandle = handle.get(); // Get raw pointer before move

    // Register the handle with the HandleManager for RAII management
    return PluginManager::GetInstance().GetHandleManager()->RegisterHandle(pluginName, std::move(handle));
}

void GameLogApi::FillGameLogApi(SPF_GameLog_API* api) {
    if (!api) return;
    api->RegisterCallback = &GameLogApi::G_RegisterCallback;
}

} // namespace Modules::API
SPF_NS_END
