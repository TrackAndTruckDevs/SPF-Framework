#pragma once

#include "SPF/Namespace.hpp"
#include <vector>
#include <mutex>
#include <functional>

SPF_NS_BEGIN
namespace Modules {

class GameLogEventManager {
public:
    using GameLogCallback = void (*)(const char* log_line, void* user_data);

    struct CallbackInfo {
        GameLogCallback callback;
        void* user_data;
    };

    /**
     * @brief Gets the singleton instance of the GameLogEventManager.
     * @return A reference to the singleton instance.
     */
    static GameLogEventManager& GetInstance();

    /**
     * @brief Registers a callback function to be invoked when a game log message is captured.
     * @param callback The function to call.
     * @param user_data A user-defined pointer passed to the callback.
     */
    void RegisterCallback(GameLogCallback callback, void* user_data);

    /**
     * @brief Unregisters a previously registered callback.
     * @param callback The function that was originally registered.
     * @param user_data The user-defined data that was originally passed.
     */
    void UnregisterCallback(GameLogCallback callback, void* user_data);
    
    /**
     * @brief Broadcasts a game log line to all registered callbacks.
     * @param log_line The captured log message.
     */
    void Broadcast(const char* log_line);

private:
    GameLogEventManager() = default;
    ~GameLogEventManager() = default;
    GameLogEventManager(const GameLogEventManager&) = delete;
    GameLogEventManager& operator=(const GameLogEventManager&) = delete;

    std::vector<CallbackInfo> m_callbacks;
    mutable std::mutex m_mutex;
};

} // namespace Modules
SPF_NS_END
