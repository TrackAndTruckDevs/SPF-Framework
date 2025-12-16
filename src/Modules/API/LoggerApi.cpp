#include "SPF/Modules/API/LoggerApi.hpp"
#include "SPF/Modules/PluginManager.hpp" // For accessing the singleton
#include "SPF/Modules/HandleManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Handles/LoggerHandle.hpp"

#include <fmt/core.h>
#include <fmt/format.h>
#include <cstdarg> // For va_list

SPF_NS_BEGIN
namespace Modules::API {

using namespace SPF::Logging;
using namespace SPF::Handles;

// --- C-API Trampoline Implementations ---

SPF_Logger_Handle* LoggerApi::L_GetLogger(const char* pluginName) {
    if (!pluginName) return nullptr;

    auto& pluginManager = PluginManager::GetInstance();
    auto* handleManager = pluginManager.GetHandleManager();
    if (!handleManager) return nullptr;

    auto logger = LoggerFactory::GetInstance().GetLogger(pluginName);
    auto handle = std::make_unique<LoggerHandle>(logger);
    return reinterpret_cast<SPF_Logger_Handle*>(handleManager->RegisterHandle(pluginName, std::move(handle)));
}

void LoggerApi::L_Log(SPF_Logger_Handle* handle, SPF_LogLevel level, const char* message) {
    auto* loggerHandle = reinterpret_cast<LoggerHandle*>(handle);
    if (loggerHandle && loggerHandle->logger && message) {
        loggerHandle->logger->Log(static_cast<LogLevel>(level), message);
    }
}

void LoggerApi::L_SetLevel(SPF_Logger_Handle* handle, SPF_LogLevel level) {
    auto* loggerHandle = reinterpret_cast<LoggerHandle*>(handle);
    if (loggerHandle && loggerHandle->logger) {
        loggerHandle->logger->SetLevel(static_cast<LogLevel>(level));
    }
}

SPF_LogLevel LoggerApi::L_GetLevel(SPF_Logger_Handle* handle) {
    auto* loggerHandle = reinterpret_cast<LoggerHandle*>(handle);
    if (loggerHandle && loggerHandle->logger) {
        return static_cast<SPF_LogLevel>(loggerHandle->logger->GetLevel());
    }
    return SPF_LOG_CRITICAL;
}

void LoggerApi::L_LogThrottled(SPF_Logger_Handle* handle, SPF_LogLevel level, const char* throttle_key, uint32_t throttle_ms, const char* message) {
    auto* loggerHandle = reinterpret_cast<LoggerHandle*>(handle);
    if (loggerHandle && loggerHandle->logger && message) {
        loggerHandle->logger->LogThrottledManual(static_cast<LogLevel>(level), throttle_key, std::chrono::milliseconds(throttle_ms), message);
    }
}

void LoggerApi::FillLoggerApi(SPF_Logger_API* api) {
    if (!api) return;

    api->GetLogger = &LoggerApi::L_GetLogger;
    api->Log = &LoggerApi::L_Log;
    api->SetLevel = &LoggerApi::L_SetLevel;
    api->GetLevel = &LoggerApi::L_GetLevel;
    api->LogThrottled = &LoggerApi::L_LogThrottled;
}

} // namespace Modules::API
SPF_NS_END
