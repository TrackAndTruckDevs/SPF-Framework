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

  static SPF_Logger_Handle* L_GetLogger(const char* pluginName);
  static void L_Log(SPF_Logger_Handle* handle, SPF_LogLevel level, const char* message);
  static void L_SetLevel(SPF_Logger_Handle* handle, SPF_LogLevel level);
  static SPF_LogLevel L_GetLevel(SPF_Logger_Handle* handle);
  static void L_LogThrottled(SPF_Logger_Handle* handle, SPF_LogLevel level, const char* throttle_key, uint32_t throttle_ms, const char* message);
};

}  // namespace Modules::API
SPF_NS_END
