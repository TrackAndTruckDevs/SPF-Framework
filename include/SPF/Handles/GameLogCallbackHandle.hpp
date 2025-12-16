#pragma once

#include "SPF/Handles/IHandle.hpp"
#include "SPF/Modules/GameLogEventManager.hpp" // For GameLogCallback and UnregisterCallback

SPF_NS_BEGIN
namespace Handles {

/**
 * @brief RAII handle for a registered GameLog callback.
 *
 * This handle ensures that a GameLog callback is automatically unregistered
 * when the handle is destroyed, preventing dangling pointers and resource leaks.
 */
struct GameLogCallbackHandle : public IHandle {
    // Using GameLogEventManager's callback type for consistency
    using GameLogCallback = Modules::GameLogEventManager::GameLogCallback;

    GameLogCallback callback;
    void* user_data;

    /**
     * @brief Constructs a GameLogCallbackHandle.
     * @param callback The callback function pointer.
     * @param user_data The user-defined data associated with the callback.
     */
    GameLogCallbackHandle(GameLogCallback callback, void* user_data);

    /**
     * @brief Destroys the GameLogCallbackHandle and unregisters the callback.
     */
    ~GameLogCallbackHandle() override;
};

} // namespace Handles
SPF_NS_END
