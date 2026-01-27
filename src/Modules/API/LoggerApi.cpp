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

SPF_Logger_Handle* LoggerApi::Log_GetContext(const char* pluginName) {
    if (!pluginName) return nullptr;

    auto& pluginManager = PluginManager::GetInstance();
    auto* handleManager = pluginManager.GetHandleManager();
    if (!handleManager) return nullptr;

    auto logger = LoggerFactory::GetInstance().GetLogger(pluginName);
    auto handle = std::make_unique<LoggerHandle>(logger);
    return reinterpret_cast<SPF_Logger_Handle*>(handleManager->RegisterHandle(pluginName, std::move(handle)));
}

void LoggerApi::Log(SPF_Logger_Handle* h, SPF_LogLevel level, const char* message) {
    auto* loggerHandle = reinterpret_cast<LoggerHandle*>(h);
    if (loggerHandle && loggerHandle->logger && message) {
        loggerHandle->logger->Log(static_cast<LogLevel>(level), message);
    }
}

void LoggerApi::Log_SetLevel(SPF_Logger_Handle* h, SPF_LogLevel level) {
    auto* loggerHandle = reinterpret_cast<LoggerHandle*>(h);
    if (loggerHandle && loggerHandle->logger) {
        loggerHandle->logger->SetLevel(static_cast<LogLevel>(level));
    }
}

SPF_LogLevel LoggerApi::Log_GetLevel(SPF_Logger_Handle* h) {
    auto* loggerHandle = reinterpret_cast<LoggerHandle*>(h);
    if (loggerHandle && loggerHandle->logger) {
        return static_cast<SPF_LogLevel>(loggerHandle->logger->GetLevel());
    }
    return SPF_LOG_CRITICAL;
}

void LoggerApi::LogThrottled(SPF_Logger_Handle* h, SPF_LogLevel level, const char* throttle_key, uint32_t throttle_ms, const char* message) {
    auto* loggerHandle = reinterpret_cast<LoggerHandle*>(h);
    if (loggerHandle && loggerHandle->logger && message) {
        loggerHandle->logger->LogThrottledManual(static_cast<LogLevel>(level), throttle_key, std::chrono::milliseconds(throttle_ms), message);
    }
}

void LoggerApi::FillLoggerApi(SPF_Logger_API* api) {
    if (!api) return;

    api->Log_GetContext = &LoggerApi::Log_GetContext;
    api->Log = &LoggerApi::Log;
    api->Log_SetLevel = &LoggerApi::Log_SetLevel;
    api->Log_GetLevel = &LoggerApi::Log_GetLevel;
    api->LogThrottled = &LoggerApi::LogThrottled;
}

} // namespace Modules::API
SPF_NS_END