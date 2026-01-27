#pragma once

#include "SPF/SPF_API/SPF_Logger_API.h"
#include "SPF/Namespace.hpp"

#include <cstdint>

SPF_NS_BEGIN
namespace Modules::API {

class LoggerApi {
 public:
  /**
   * @brief Fills the provided SPF_Logger_API struct with pointers to the C-style trampoline functions.
   * @param api The struct to fill.
   */
  static void FillLoggerApi(SPF_Logger_API* api);

 private:
  // --- C-API Trampoline Implementations ---

  static SPF_Logger_Handle* Log_GetContext(const char* pluginName);
  static void Log(SPF_Logger_Handle* h, SPF_LogLevel level, const char* message);
  static void Log_SetLevel(SPF_Logger_Handle* h, SPF_LogLevel level);
  static SPF_LogLevel Log_GetLevel(SPF_Logger_Handle* h);
  static void LogThrottled(SPF_Logger_Handle* h, SPF_LogLevel level, const char* throttle_key, uint32_t throttle_ms, const char* message);
};

}  // namespace Modules::API
SPF_NS_END