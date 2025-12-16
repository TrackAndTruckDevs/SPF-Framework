#include "SPF/Modules/GameLogEventManager.hpp"

SPF_NS_BEGIN
namespace Modules {

GameLogEventManager& GameLogEventManager::GetInstance() {
    static GameLogEventManager instance;
    return instance;
}

void GameLogEventManager::RegisterCallback(GameLogCallback callback, void* user_data) {
    if (!callback) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_callbacks.push_back({callback, user_data});
}

void GameLogEventManager::Broadcast(const char* log_line) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& info : m_callbacks) {
        info.callback(log_line, info.user_data);
    }
}

void GameLogEventManager::UnregisterCallback(GameLogCallback callback, void* user_data) {
    if (!callback) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_callbacks.erase(
        std::remove_if(m_callbacks.begin(), m_callbacks.end(),
            [callback, user_data](const CallbackInfo& info) {
                return info.callback == callback && info.user_data == user_data;
            }),
        m_callbacks.end());
}

} // namespace Modules
SPF_NS_END
