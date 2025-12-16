#include "SPF/Handles/GameLogCallbackHandle.hpp"
#include "SPF/Modules/GameLogEventManager.hpp" // For GetInstance()

SPF_NS_BEGIN
namespace Handles {

GameLogCallbackHandle::GameLogCallbackHandle(GameLogCallback callback, void* user_data)
    : callback(callback), user_data(user_data) {
    // No action needed here, registration happens in GameLogApi::T_RegisterCallback
}

GameLogCallbackHandle::~GameLogCallbackHandle() {
    // Unregister the callback when the handle is destroyed
    Modules::GameLogEventManager::GetInstance().UnregisterCallback(callback, user_data);
}

} // namespace Handles
SPF_NS_END
