/**                                                                                               
* @file SPF_Formatting_API.h                                                                          
* @brief API for safe, cross-DLL string formatting.
*                                                                                                 
* @details This API provides a reliable way for plugins to format strings using 
* printf-style arguments. It acts as a safe wrapper around 'vsnprintf' that handles 
* variadic arguments within the framework's memory space, preventing crashes 
* caused by passing 'va_list' or variadic arguments across DLL boundaries.
*                                                                                                 
* Plugins should always use this API for formatting log messages, UI text, 
* or any dynamic strings intended for display.                                       
*                                                                                                 
* ================================================================================================
* KEY CONCEPTS                                                                                    
* ================================================================================================
*                                                                                                 
* 1. **Safety First**: Directly passing variadic arguments (...) to functions in another DLL 
*    is unsafe. This API centralizes formatting logic to ensure stability.
*                                                                                                 
* 2. **Buffer Management**: Always provide an appropriately sized buffer. If the 
*    formatted string is longer than the buffer, it will be safely truncated.
*                                                                                                 
* 3. **vsnprintf Compatibility**: The behavior, including return values and 
*    format specifiers, follows the standard C 'vsnprintf' specification.
*                                                                                                 
* ================================================================================================
* USAGE EXAMPLE (C++)                                                                             
* ================================================================================================
* @code                                                                                           
* // Assume 'api' is the SPF_Plugin_API* passed to OnLoad
* SPF_Formatting_API* fmt = api->formatting;
* SPF_Logger_API* log = api->logger;
* 
* // Get logger handle once (e.g. in OnLoad)
* SPF_Logger_Handle* hLog = log->Log_GetContext("MyPlugin");
*
* void LogValue(int value) {
*     char buffer[256];
*     
*     // Format the string safely
*     fmt->Fmt_Format(buffer, sizeof(buffer), "The current value is: %d", value);
*
*     // Use the result with the logger API
*     log->Log(hLog, SPF_LOG_INFO, buffer);
* }
* @endcode                                                                                        
*/ 

#pragma once

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct SPF_Formatting_API
 * @brief Table of function pointers for string formatting operations.
 */
typedef struct SPF_Formatting_API {
    /**
     * @brief Formats a string using printf-style arguments and stores it in a buffer.
     *
     * @details This function takes a format string and a variable number of arguments, 
     *          formats them according to standard printf rules, and writes the result 
     *          into the provided buffer. It ensures the output is always null-terminated 
     *          and does not exceed the specified buffer size.
     *
     * @param[out] buffer A pointer to the character buffer to receive the result.
     * @param buffer_size The total size of the destination buffer in bytes.
     * @param format The printf-style format string (e.g., "Value: %d").
     * @param ... The variable arguments corresponding to the format specifiers.
     * @return The number of characters that would have been written if the buffer 
     *         was large enough (excluding the null terminator). A negative value 
     *         indicates a formatting error.
     */
    int (*Fmt_Format)(char* buffer, size_t buffer_size, const char* format, ...);

} SPF_Formatting_API;


#ifdef __cplusplus
}
#endif